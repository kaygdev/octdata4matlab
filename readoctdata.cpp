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

#include "mex.h"

#include <cmath>
#include <limits>
#include <boost/type_index.hpp>
#include<boost/lexical_cast.hpp>
#include<string>

#include <opencv2/opencv.hpp>

#include <octdata/octfileread.h>
#include <octdata/filereadoptions.h>
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
	mxArray* writeParameter(const S& structure)
	{
		ParameterToOptions pto;
		structure.getSetParameter(pto);
		return pto.getMxOptions();
	}

	// general export methods
	mxArray* convertSlo(const OctData::SloImage& slo)
	{
		ParameterToOptions pto;
		pto.addMxArray("data", writeParameter(slo));
		pto.addMxArray("image", convertMatrix<uint8_t>(slo.getImage()));

		return pto.getMxOptions();
	}

	mxArray* convertSegmentation(const OctData::Segmentationlines& seglines)
	{
		ParameterToOptions pto;
		for(OctData::Segmentationlines::SegmentlineType type : OctData::Segmentationlines::getSegmentlineTypes())
		{
			const OctData::Segmentationlines::Segmentline& seg = seglines.getSegmentLine(type);
			if(!seg.empty())
				pto(OctData::Segmentationlines::getSegmentlineName(type), seg);
		}

		return pto.getMxOptions();
	}

	mxArray* convertBScan(const std::shared_ptr<const OctData::BScan>& bscan)
	{
		if(!bscan)
			return nullptr;

		ParameterToOptions pto;

		pto.addMxArray("data", writeParameter(*bscan));

		if(!bscan->getImage().empty())
			pto.addMxArray("image", convertMatrix<uint8_t>(bscan->getImage()));
		if(!bscan->getAngioImage().empty())
			pto.addMxArray("imageAngio", convertMatrix<uint8_t>(bscan->getAngioImage()));

		pto.addMxArray("segmentation", convertSegmentation(bscan->getSegmentLines()));

		return pto.getMxOptions();
	}

	template<typename S>
	mxArray* convertStructure(const S& structure)
	{
		static const std::string structureName = getSubStructureName<S>();

		ParameterToOptions pto;
		pto.addMxArray("data", writeParameter(structure));

		for(typename S::SubstructurePair const& subStructPair : structure)
		{
			mxArray* subStruct = convertStructure(*subStructPair.second);
			std::string subStructName = structureName + '_' + boost::lexical_cast<std::string>(subStructPair.first);
			pto.addMxArray(subStructName, subStruct);
		}
		return pto.getMxOptions();
	}


	template<>
	mxArray* convertStructure<OctData::Series>(const OctData::Series& series)
	{
		ParameterToOptions pto;
		pto.addMxArray("data", writeParameter(series));

		pto.addMxArray("slo", convertSlo(series.getSloImage()));

		const OctData::Series::BScanList& bscans = series.getBScans();

		const uint32_t dirLength = static_cast<uint32_t>(bscans.size());
		mxArray* mxarr = mxCreateCellMatrix(1, dirLength);
		for(uint32_t i = 0; i < dirLength; ++i)
			mxSetCell(mxarr, i, convertBScan(bscans[i]));

		pto.addMxArray("bscans", mxarr);

		return pto.getMxOptions();
	}
}


mxArray* readOctData(const mxArray* mxOptions, const std::string& filename)
{
	// Load Options
	OctData::FileReadOptions options;

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

	OctData::OCT oct = OctData::OctFileRead::openFile(filename, options);

	mxArray* matlabOut = convertStructure(oct);


	return matlabOut;
}



void mexFunction(int            nlhs
               , mxArray*       plhs[]
               , int            nrhs
               , const mxArray* prhs[])
{
	/* Check for proper number of arguments */
	if(nrhs > 2 || nrhs < 1)
	{
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargin", "MEXCPP requires 1 or 2 input arguments (filename, options[struct])");
		return;
	}
	else if(nlhs > 1)
	{
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargout", "MEXCPP requires only one output argument.");
		return;
	}


	if(!mxIsChar(prhs[0]))
	{
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargin", "requires filename");
		return;
	}


	const mxArray* mxOptions = nullptr;
	if(nrhs == 2)
		mxOptions = prhs[1];


	std::string filename = getScalarConvert<std::string>(prhs[0]);
	plhs[0] = readOctData(mxOptions, filename);

	return;
}
