
#include <cstdint>
#include <iostream>
#include <math.h>
#include "mex.h"

#include <octdata/octfileread.h>
#include <octdata/datastruct/oct.h>

#include <octdata/datastruct/sloimage.h>
#include <octdata/datastruct/bscan.h>

#include <opencv/cv.h>

template<typename T>
void createMatlabMatrix(mxArray*& mat, std::size_t rows, std::size_t cols);

template<>
void createMatlabMatrix<uint8_t>(mxArray*& mat, std::size_t rows, std::size_t cols)
{
	mat = mxCreateNumericMatrix(rows, cols, mxUINT8_CLASS, mxREAL);
}

template<typename T>
void copyMatrix(const cv::Mat& cvMat, T* matlabPtr)
{
	if(!matlabPtr)
		return;

	std::size_t sizeCols = cvMat.cols;
	std::size_t sizeRows = cvMat.rows;

	for(std::size_t i = 0; i < sizeRows; ++i)
	{
		T* matlabLine = matlabPtr + i;
		const T* ptr = cvMat.ptr<T>(i);
		for(std::size_t j = 0; j < sizeCols; ++j)
		{
		//	*(matlabPtr + j*sizeRows + i) = *ptr;
			*matlabLine = *ptr;
			matlabLine += sizeRows;
			++ptr;
		}
	}
}

template<typename T>
void createCopyMatrix(const cv::Mat& cvMat, mxArray*& matlabMat)
{
	if(matlabMat)
		return;

	std::size_t sizeCols = cvMat.cols;
	std::size_t sizeRows = cvMat.rows;
	createMatlabMatrix<T>(matlabMat, sizeRows, sizeCols);

	if(!matlabMat)
		return;

	T* matlabPtr = reinterpret_cast<T*>(mxGetPr(matlabMat));
/*
	memcpy(matlabPtr, cvMat.data, sizeCols*sizeRows);

	*/


	for(std::size_t i = 0; i < sizeRows; ++i)
	{
		const T* ptr = cvMat.ptr<T>(i);
		for(std::size_t j = 0; j < sizeCols; ++j)
		{
			*(matlabPtr + j*sizeRows + i) = *ptr;
			++ptr;
		}
	}
}


void mexFunction(
		 int          nlhs,
		 mxArray      *plhs[],
		 int          nrhs,
		 const mxArray *prhs[]
		 )
	{
	/* Check for proper number of arguments */

	if (nrhs != 1) {
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargin",
				"MEXCPP requires only one input arguments.");
	} else if (nlhs != 1) {
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargout",
				"MEXCPP requires no output argument.");
	}

	std::cout << "isChar: " << mxIsChar(prhs[0]) << '\n';
	std::cout << "mxGetM(): " << mxGetM(prhs[0]) << '\n';
	std::cout << "mxGetN(): " << mxGetN(prhs[0]) << '\n';


	if(mxIsChar(prhs[0]))
	{
		mxChar* fnPtr = (mxChar*) mxGetPr(prhs[0]);
		std::size_t fnLength = mxGetN(prhs[0]);
		std::string filename(fnPtr, fnPtr+fnLength);

		std::cout << "filename: " << filename << '\n';

		OctData::OCT oct = OctData::OctFileRead::openFile(filename);


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

		// SLO
		const OctData::SloImage& sloImg = series->getSloImage();
		const cv::Mat& sloCvMat = sloImg.getImage();

		mxArray* sloMatlab = nullptr;
		createCopyMatrix<uint8_t>(sloCvMat, sloMatlab);



		// const OctData::Series::BScanList& bscans = series->getBScans();

		// BScan
		const OctData::BScan* bscan = series->getBScan(0);
		const cv::Mat& bscanCvMat = bscan->getImage();

		mxArray* bscanMatlab = nullptr;
		createCopyMatrix<uint8_t>(bscanCvMat, bscanMatlab);

		mxSetFieldByNumber(octMatlabStruct, 0, 0, sloMatlab);
		mxSetFieldByNumber(octMatlabStruct, 0, 1, bscanMatlab);

		plhs[0] = octMatlabStruct;



	}
	else
	{
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargin", "requires filename");
	}

	
	return;
}
