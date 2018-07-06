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

		return true;

// 		boost::optional<const bpt::ptree&> sloNode = tree.get_child_optional("slo");
// 		if(sloNode)
// 			series.takeSloImage(readSlo(*sloNode, zipfile));
//
// 		if(op.readBScans)
// 			return readBScanList(tree, zipfile, series, callback);
// 		else
// 			return true;
	}
	/*
	template<typename S>
	mxArray* convertStructure(const mxArray* matlabStruct, S& structure)
	{
		matlabStruct(matlabStruct, structure);

		std::string structureName = getSubStructureName<S>();

		for(typename S::SubstructurePair const& subStructPair : structure)
		{
			mxArray* subStruct = convertStructure(*subStructPair.second);
			std::string subStructName = structureName + '_' + boost::lexical_cast<std::string>(subStructPair.first);
			pto.addMxArray(subStructName, subStruct);

			mxArray* mxOpt = mxGetField(matlabStruct, 0, subStructName.c_str());
			if(mxOpt)
				return getScalarConvert<T>(mxOpt);
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
	*/
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
