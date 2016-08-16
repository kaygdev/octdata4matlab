
#include <cstdint>
#include <iostream>
#include <math.h>
#include "mex.h"

#include <octdata/octfileread.h>
#include <octdata/datastruct/oct.h>

#include <octdata/datastruct/sloimage.h>
#include <octdata/datastruct/bscan.h>

#include <limits>

#include <opencv/cv.h>

#include "matlab_types.h"


template<typename T>
void copyMatrix(const cv::Mat& cvMat, T* matlabPtr)
{
	if(!matlabPtr)
		return;

	std::size_t sizeCols = cvMat.cols;
	std::size_t sizeRows = cvMat.rows;

	// copy transpose matrix because opencv's structure is row based and matlab's structure is col based
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
	matlabMat = mxCreateNumericMatrix(sizeRows, sizeCols, MatlabType<T>::classID, mxREAL);

	if(!matlabMat)
		return;

	T* matlabPtr = reinterpret_cast<T*>(mxGetPr(matlabMat));
	copyMatrix<T>(cvMat, matlabPtr);
}

template<typename T>
mxArray* createCopyMatrix(const cv::Mat& cvMat)
{
	mxArray* matlabMat = nullptr;
	createCopyMatrix<T>(cvMat, matlabMat);
	return matlabMat;
}

template<typename T>
void createMatlabVector(const std::vector<T>& vec, mxArray*& matlabMat)
{
	if(matlabMat)
		return;

	std::size_t size = vec.size();
	matlabMat = mxCreateNumericMatrix(size, 1, MatlabType<T>::classID, mxREAL);

	if(!matlabMat)
		return;

	T* matlabPtr = reinterpret_cast<T*>(mxGetPr(matlabMat));
	for(T value : vec)
	{
		*matlabPtr = value;
		++matlabPtr;
	}
}

template<typename T>
void createMatlabArray(const T* vec, mxArray*& matlabMat, std::size_t size)
{
	if(matlabMat)
		return;

	matlabMat = mxCreateNumericMatrix(size, 1, MatlabType<T>::classID, mxREAL);

	if(!matlabMat)
		return;

	T* matlabPtr = reinterpret_cast<T*>(mxGetPr(matlabMat));
	const T* vecEnd = vec + size;
	while(vec != vecEnd)
	{
		*matlabPtr = *vec;
		++matlabPtr;
		++vec;
	}
}

template<typename T>
mxArray* createMatlabArray(const T* vec, std::size_t size)
{
	mxArray* matlabMat = nullptr;
	createMatlabArray<T>(vec, matlabMat, size);
	return matlabMat;
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

	if(mxIsChar(prhs[0]))
	{
		mxChar* fnPtr = (mxChar*) mxGetPr(prhs[0]);
		std::size_t fnLength = mxGetN(prhs[0]);
		std::string filename(fnPtr, fnPtr+fnLength);

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

		//-------
		// SLO
		//-------
		const char* sloFieldnames[] = {"Rows", "Columns", "img"}; //, "PixelSpacing"};
		mxArray* octSloMatlabStruct = mxCreateStructMatrix(1, 1, sizeof(sloFieldnames)/sizeof(sloFieldnames[0]), sloFieldnames);

		const OctData::SloImage& sloImg = series->getSloImage();
		const cv::Mat& sloCvMat = sloImg.getImage();
		std::size_t sloRows = sloCvMat.rows;
		std::size_t sloCols = sloCvMat.cols;

		mxSetFieldByNumber(octSloMatlabStruct, 0, 0, createMatlabArray<std::size_t>(&sloRows, 1));
		mxSetFieldByNumber(octSloMatlabStruct, 0, 1, createMatlabArray<std::size_t>(&sloCols, 1));
		mxSetFieldByNumber(octSloMatlabStruct, 0, 2, createCopyMatrix<uint8_t>(sloCvMat));
		//

		//-------
		// BScan
		//-------
		const char* serieFieldnames[] = {"Rows", "Columns", "NumberOfFrames", "bscans"}; //, "PixelSpacing"};
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
		mxArray* bscanArray = mxCreateNumericArray(dimNum, dims, mxUINT8_CLASS, mxREAL);

		// convert cvMats to MatlabArray
		uint8_t* matlabPtr = reinterpret_cast<uint8_t*>(mxGetPr(bscanArray));
		for(const OctData::BScan* bscan : bscans)
		{
			copyMatrix<uint8_t>(bscan->getImage(), matlabPtr);
			matlabPtr += firstBscan.rows * firstBscan.cols;
		}

		// fill Serie struct
		mxSetFieldByNumber(octSerieMatlabStruct, 0, 3, bscanArray);
		// mxSetFieldByNumber(octSerieMatlabStruct, 0, 4, bscanMatlab);


		//
		// fill OCT structure
		//
		mxSetFieldByNumber(octMatlabStruct, 0, 0, octSloMatlabStruct);
		mxSetFieldByNumber(octMatlabStruct, 0, 1, octSerieMatlabStruct);

		plhs[0] = octMatlabStruct;


//

	}
	else
	{
		mexErrMsgIdAndTxt("MATLAB:mexcpp:nargin", "requires filename");
	}

	
	return;
}
