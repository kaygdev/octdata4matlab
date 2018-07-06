#include<boost/type_index.hpp>
#include<boost/lexical_cast.hpp>

#include <octdata/octfileread.h>
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


	OctData::SloImage* readSlo(const mxArray* sloNode)
	{
		cv::Mat sloImage = convertImage(sloNode, "image");
		if(sloImage.empty())
			return nullptr;

		OctData::SloImage* slo = new OctData::SloImage();
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

	OctData::BScan* readBScan(const mxArray* bscanNode)
	{
		cv::Mat bscanImg = convertImage(bscanNode, "image");
		if(bscanImg.empty())
			return nullptr;

		cv::Mat imageAngio = convertImage(bscanNode, "angioImage");

		OctData::BScan::Data bscanData;


		const mxArray* segNode = mxGetField(bscanNode, 0, "segmentation");
		if(segNode)
			readSegmentation(segNode, bscanData.segmentationslines);

		OctData::BScan* bscan = new OctData::BScan(bscanImg, bscanData);

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

			OctData::BScan* bscan = readBScan(bscanNode);
			if(bscan)
				series.takeBScan(bscan);
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




void writeOctData(const mxArray* mxOptions, const mxArray* data, const std::string& filename)
{
	OctData::OCT oct;
	readStructure(data, oct);

	OctData::OctFileRead::writeFile(filename, oct);
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
	else if(nlhs != 0)
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
	if(nrhs == 3)
		mxOptions = prhs[2];


	std::string filename = getScalarConvert<std::string>(prhs[0]);
	mexPrintf("open: %s\n", filename.c_str());
	writeOctData(mxOptions, prhs[1], filename);

	return;
}
