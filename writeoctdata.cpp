/*
 * Copyright (c) 2018 Kay Gawlik <kaydev@amarunet.de> <kay.gawlik@beuth-hochschule.de> <kay.gawlik@charite.de>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include<boost/type_index.hpp>
#include<boost/lexical_cast.hpp>

#include <octdata/octfileread.h>
#include <octdata/filewriteoptions.h>
#include <octdata/datastruct/oct.h>
#include <octdata/datastruct/sloimage.h>
#include <octdata/datastruct/bscan.h>

#include "helper/matlab_helper.h"
#include "helper/matlab_types.h"
#include "helper/opencv_helper.h"

namespace
{
	template<typename S>
	std::string getSubStructureName()
	{
		std::string name = boost::typeindex::type_id<typename S::SubstructureType>().pretty_name();
		std::size_t namePos = name.rfind(':');
		if(namePos > 0)
			++namePos;
		return name.substr(namePos, name.size() - namePos);
	}

	template<typename S>
	void readDataNode(const mxArray* matlabStruct, S& structure)
	{
		const mxArray* dataNode = mxGetField(matlabStruct, 0, "data");
		if(dataNode)
		{
			ParameterFromOptions pto(dataNode);
			structure.getSetParameter(pto);
		}
	}

	// general convert methods
	cv::Mat convertImage(const mxArray* matlabStruct, const char* imageStr)
	{
		const mxArray* imageNode = mxGetField(matlabStruct, 0, imageStr);
		return convertMatrix<uint8_t>(imageNode);
	}


	std::unique_ptr<OctData::SloImage> readSlo(const mxArray* sloNode)
	{
		cv::Mat sloImage = convertImage(sloNode, "image");
		if(sloImage.empty())
			return nullptr;

		std::unique_ptr<OctData::SloImage> slo = std::make_unique<OctData::SloImage>();
		slo->setImage(sloImage);
		readDataNode(sloNode, *slo);
		return slo;
	}

	void readSegmentation(const mxArray* segNode, OctData::Segmentationlines& seglines)
	{
		ParameterFromOptions get(segNode);
		for(OctData::Segmentationlines::SegmentlineType type : OctData::Segmentationlines::getSegmentlineTypes())
		{
			OctData::Segmentationlines::Segmentline& seg = seglines.getSegmentLine(type);
			get(OctData::Segmentationlines::getSegmentlineName(type), seg);
		}
	}

	std::shared_ptr<OctData::BScan> readBScan(const mxArray* bscanNode)
	{
		cv::Mat bscanImg = convertImage(bscanNode, "image");
		if(bscanImg.empty())
			return nullptr;

		cv::Mat imageAngio = convertImage(bscanNode, "angioImage");

		OctData::BScan::Data bscanData;


		const mxArray* segNode = mxGetField(bscanNode, 0, "segmentation");
		if(segNode)
			readSegmentation(segNode, bscanData.segmentationslines);

		std::shared_ptr<OctData::BScan> bscan = std::make_shared<OctData::BScan>(bscanImg, bscanData);

		if(!imageAngio.empty())
			bscan->setAngioImage(imageAngio);

		readDataNode(bscanNode, *bscan);
		return bscan;
	}


	bool readBScanList(const mxArray* seriesNode, OctData::Series& series)
	{
		const mxArray* bscansNode = mxGetField(seriesNode, 0, "bscans");
		if(!bscansNode || !mxIsCell(bscansNode))
			return false;

		const mwSize* numSubStruct = mxGetDimensions(bscansNode);
		const mwSize numBScans = numSubStruct[0] * numSubStruct[1];

		for(mwSize i = 0; i < numBScans; ++i)
		{
		    const mxArray* bscanNode = mxGetCell(bscansNode, i);

			std::shared_ptr<OctData::BScan> bscan = readBScan(bscanNode);
			if(bscan)
				series.addBScan(std::move(bscan));
		}

		return true;
	}

	template<typename S>
	bool readStructure(const mxArray* matlabStruct, S& structure)
	{
		static const std::string subStructureName = getSubStructureName<S>();

		readDataNode(matlabStruct, structure);

		bool result = true;

		const int numSubStruct = mxGetNumberOfFields(matlabStruct);
		for(int i = 0; i < numSubStruct; ++i)
		{
			const char* subStructName = mxGetFieldNameByNumber(matlabStruct, i);
			if(memcmp(subStructName, subStructureName.c_str(), subStructureName.size()) == 0)
			{
				std::string subNumberStr = std::string(subStructName).substr(subStructureName.size()+1);
				try
				{
					const int id = boost::lexical_cast<int>(subNumberStr);
					const mxArray* subArray = mxGetFieldByNumber(matlabStruct, 0, i);
					result &= readStructure(subArray, structure.getInsertId(id));
				}
				catch(...)
				{
				}
			}
		}

		return result;
	}


	template<>
	bool readStructure<OctData::Series>(const mxArray* matlabStruct, OctData::Series& series)
	{
		readDataNode(matlabStruct, series);


		const mxArray* sloNode = mxGetField(matlabStruct, 0, "slo");
		if(sloNode)
			series.takeSloImage(readSlo(sloNode));

		return readBScanList(matlabStruct, series);
	}

}




mxArray* writeOctData(const mxArray* mxOptions, const mxArray* data, const std::string& filename)
{
	// Load Options
	OctData::FileWriteOptions options;

	if(mxOptions && mxIsStruct(mxOptions))
	{
		ParameterFromOptions paraFromOptions(mxOptions);
		options.getSetParameter(paraFromOptions);
	}

	if(filename.empty())
	{
		ParameterToOptions paraToOptions;
		options.getSetParameter(paraToOptions);
		return paraToOptions.getMxOptions();
	}


	OctData::OCT oct;
	readStructure(data, oct);

	OctData::OctFileRead::writeFile(filename, oct, options);

	return nullptr;
}



void mexFunction(int            nlhs
               , mxArray*       plhs[]
               , int            nrhs
               , const mxArray* prhs[])
{
	/* Check for proper number of arguments */
	if(nrhs > 3 || nrhs < 2)
	{
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargin", "MEXCPP requires 2 or 3 input arguments (filename, data, options[struct])");
		return;
	}
	if(!mxIsChar(prhs[0]))
	{
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargin", "requires filename");
		return;
	}

	std::string filename = getScalarConvert<std::string>(prhs[0]);

	if(nlhs != 0 && !filename.empty())
	{
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargout", "MEXCPP requires only one output argument.");
		return;
	}


	const mxArray* mxOptions = nullptr;
	if(nrhs == 3)
		mxOptions = prhs[2];


	plhs[0] = writeOctData(mxOptions, prhs[1], filename);

	return;
}
