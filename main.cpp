#include <cstdint>
#include <iostream>
#include <math.h>
#include "mex.h"

#include <octdata/octfileread.h>
#include <octdata/filereadoptions.h>
#include <octdata/datastruct/oct.h>

#include <octdata/datastruct/sloimage.h>
#include <octdata/datastruct/bscan.h>

#include <limits>

#include <opencv/cv.h>

#include "helper/matlab_helper.h"
#include "helper/matlab_types.h"
#include "helper/opencv_helper.h"


#include"readoctdata.cpp"



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
	mexPrintf("open: %s\n", filename.c_str());
	plhs[0] = readOctData(mxOptions, filename);

	return;

#if false
	OctData::OCT oct = OctData::OctFileRead::openFile(filename, options);




	// TODO: bessere Datenverwertung / Fehlerbehandlung
	if(oct.size() == 0)
		throw "MarkerManager::loadImage: oct->size() == 0";
	OctData::Patient* patient = oct.begin()->second;
	if(patient->size() == 0)
		throw "MarkerManager::loadImage: patient->size() == 0";
	OctData::Study* study = patient->begin()->second;
	if(study->size() == 0)
		throw "MarkerManager::loadImage: study->size() == 0";
	OctData::Series* series = study->begin()->second;

	const char* octFieldnames[] = {"SLO", "Serie"}; //, "Meta"};

	mxArray* octMatlabStruct = mxCreateStructMatrix(1, 1, sizeof(octFieldnames)/sizeof(octFieldnames[0]), octFieldnames);

	//-------
	// SLO
	//-------
	const char* sloFieldnames[] = {"Rows", "Columns", "img", "PixelSpacing"};
	mxArray* octSloMatlabStruct = mxCreateStructMatrix(1, 1, sizeof(sloFieldnames)/sizeof(sloFieldnames[0]), sloFieldnames);

	const OctData::SloImage& sloImg = series->getSloImage();
	const cv::Mat& sloCvMat = sloImg.getImage();
	std::size_t sloRows = sloCvMat.rows;
	std::size_t sloCols = sloCvMat.cols;

	mxSetFieldByNumber(octSloMatlabStruct, 0, 0, createMatlabArray<std::size_t>(&sloRows, 1));
	mxSetFieldByNumber(octSloMatlabStruct, 0, 1, createMatlabArray<std::size_t>(&sloCols, 1));
	mxSetFieldByNumber(octSloMatlabStruct, 0, 2, createCopyMatrix<uint8_t>(sloCvMat));
	const OctData::ScaleFactor& scaleSlo = sloImg.getScaleFactor();
	double scaleSloField[] = {scaleSlo.getX(), scaleSlo.getY()};
	mxSetFieldByNumber(octSloMatlabStruct, 0, 3, createMatlabArray(scaleSloField, sizeof(scaleSloField)/sizeof(scaleSloField[0])));
	//

	//-------
	// BScan
	//-------
	const char* serieFieldnames[] = {"Rows", "Columns", "NumberOfFrames", "img", "Segmentation"}; //, "PixelSpacing"};
	mxArray* octSerieMatlabStruct = mxCreateStructMatrix(1, 1, sizeof(serieFieldnames)/sizeof(serieFieldnames[0]), serieFieldnames);

	// general data
	const OctData::Series::BScanList& bscans = series->getBScans();
	cv::Mat firstBscan = bscans.at(0)->getImage();
	std::size_t rows = firstBscan.rows;
	std::size_t cols = firstBscan.cols;
	std::size_t nrBscans = bscans.size();

	// fill basic elements
	mxSetFieldByNumber(octSerieMatlabStruct, 0, 0, createMatlabArray<std::size_t>(&rows, 1));
	mxSetFieldByNumber(octSerieMatlabStruct, 0, 1, createMatlabArray<std::size_t>(&cols, 1));
	mxSetFieldByNumber(octSerieMatlabStruct, 0, 2, createMatlabArray<std::size_t>(&nrBscans, 1));

	// get images
	const size_t dims[] = {rows, cols, nrBscans};
	const size_t dimNum = sizeof(dims)/sizeof(dims[0]);
	mxArray* bscanArray = mxCreateNumericArray(dimNum, dims, MatlabType<uint8_t>::classID, mxREAL);

	// convert cvMats to MatlabArray
	uint8_t* matlabPtr = reinterpret_cast<uint8_t*>(mxGetPr(bscanArray));
	for(const OctData::BScan* bscan : bscans)
	{
		copyMatrix<uint8_t>(bscan->getImage(), matlabPtr);
		matlabPtr += firstBscan.rows * firstBscan.cols;
	}

	// Segmentation
	typedef double SegDataType;
	const char* SegmentationFieldnames[] = {"ILM", "BM"}; //, "PixelSpacing"};
	mxArray* octSerieSegmentationMatlabStruct = mxCreateStructMatrix(1, 1, sizeof(SegmentationFieldnames)/sizeof(SegmentationFieldnames[0]), SegmentationFieldnames);
	mxArray* octSegILM = mxCreateNumericMatrix(cols, nrBscans, MatlabType<SegDataType>::classID, mxREAL);
	mxArray* octSegBM  = mxCreateNumericMatrix(cols, nrBscans, MatlabType<SegDataType>::classID, mxREAL);
	std::size_t actBscan = 0;
	for(const OctData::BScan* bscan : bscans)
	{
		copyVec2MatlabCol(bscan->getSegmentLine(OctData::Segmentationlines::SegmentlineType::ILM), actBscan, octSegILM);
		copyVec2MatlabCol(bscan->getSegmentLine(OctData::Segmentationlines::SegmentlineType::BM ), actBscan, octSegBM );
		++actBscan;
	}
	mxSetFieldByNumber(octSerieSegmentationMatlabStruct, 0, 0, octSegILM);
	mxSetFieldByNumber(octSerieSegmentationMatlabStruct, 0, 1, octSegBM);

	// fill Serie struct
	mxSetFieldByNumber(octSerieMatlabStruct, 0, 3, bscanArray);
	mxSetFieldByNumber(octSerieMatlabStruct, 0, 4, octSerieSegmentationMatlabStruct);
	// mxSetFieldByNumber(octSerieMatlabStruct, 0, 4, bscanMatlab);


	//
	// fill OCT structure
	//
	mxSetFieldByNumber(octMatlabStruct, 0, 0, octSloMatlabStruct);
	mxSetFieldByNumber(octMatlabStruct, 0, 1, octSerieMatlabStruct);

	plhs[0] = octMatlabStruct;


	
	return;


#endif
}
