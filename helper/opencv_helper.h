#pragma once

#include <opencv/cv.h>

#include "matlab_types.h"
#include "mex.h"


template<typename T>
void copyMatrixTranspose(const cv::Mat& cvMat, T* matlabPtr, std::size_t channel)
{
	const std::size_t sizeCols = cvMat.cols;
	const std::size_t sizeRows = cvMat.rows;
	const std::size_t channels = cvMat.channels();

	for(std::size_t i = 0; i < sizeRows; ++i)
	{
		T* matlabLine = matlabPtr + i;
		const T* ptr = cvMat.ptr<T>(i) + channel;
		for(std::size_t j = 0; j < sizeCols; ++j)
		{
			*matlabLine = *ptr;
			matlabLine += sizeRows;
			ptr += channels;
		}
	}
}

template<typename T>
void copyMatrix(const cv::Mat& cvMat, T* matlabPtr)
{
	if(!matlabPtr)
		return;

	const std::size_t sizeCols = cvMat.cols;
	const std::size_t sizeRows = cvMat.rows;
	const std::size_t channels = cvMat.channels();

	// copy transpose matrix because opencv's structure is row based and matlab's structure is col based
	for(std::size_t channel = 0; channel < channels; ++channel)
		copyMatrixTranspose(cvMat, matlabPtr + channel*sizeCols*sizeRows, channel);
}

template<typename T>
void createCopyMatrix(const cv::Mat& cvMat, mxArray*& matlabMat)
{
	if(matlabMat)
		return;

	const std::size_t sizeCols = cvMat.cols;
	const std::size_t sizeRows = cvMat.rows;
	const std::size_t channels = cvMat.channels();

	if(channels == 1)
		matlabMat = mxCreateNumericMatrix(sizeRows, sizeCols, MatlabType<T>::classID, mxREAL);
	else
	{
		mwSize dimsArray[] = {sizeRows, sizeCols, channels};
		matlabMat = mxCreateNumericArray(3, dimsArray, MatlabType<T>::classID, mxREAL);
	}

	if(!matlabMat)
		return;

	T* matlabPtr = reinterpret_cast<T*>(mxGetPr(matlabMat));
	copyMatrix<T>(cvMat, matlabPtr);
}


template<typename T>
mxArray* convertMatrix(const cv::Mat& cvMat)
{
	mxArray* matlabMat = nullptr;
	createCopyMatrix<T>(cvMat, matlabMat);
	return matlabMat;
}


template<typename T>
void copyMatrixTranspose(const T* matlabPtr, cv::Mat& cvMat, std::size_t channel)
{
	const std::size_t sizeCols = cvMat.cols;
	const std::size_t sizeRows = cvMat.rows;
	const std::size_t channels = cvMat.channels();

	for(std::size_t i = 0; i < sizeRows; ++i)
	{
		const T* matlabLine = matlabPtr + i;
		T* ptr = cvMat.ptr<T>(i) + channel;
		for(std::size_t j = 0; j < sizeCols; ++j)
		{
			*ptr = *matlabLine;
			matlabLine += sizeRows;
			ptr += channels;
		}
	}
}

template<typename T>
void copyMatrix(const mxArray* matlabMat, cv::Mat& cvMat)
{
	if(!matlabMat)
		return;

	const std::size_t sizeCols = cvMat.cols;
	const std::size_t sizeRows = cvMat.rows;
	const std::size_t channels = cvMat.channels();

	const T* matlabPtr = reinterpret_cast<const T*>(mxGetPr(matlabMat));
	// copy transpose matrix because opencv's structure is row based and matlab's structure is col based
	for(std::size_t channel = 0; channel < channels; ++channel)
		copyMatrixTranspose(matlabPtr + channel*sizeCols*sizeRows, cvMat, channel);
}


template<typename T>
cv::Mat convertMatrix(const mxArray* matlabMat)
{
	if(!matlabMat)
		return cv::Mat();

	const mwSize numDims = mxGetNumberOfDimensions(matlabMat);
	const mwSize* dims   = mxGetDimensions(matlabMat);

	if(numDims != 2 && numDims != 3)
		return cv::Mat(); // TODO error message

	int channels = numDims==3?dims[2]:1;

	cv::Mat cvMat(dims[0], dims[1], CV_MAKETYPE(cv::DataType<uint8_t>::type, channels));
	copyMatrix<T>(matlabMat, cvMat);
	return cvMat;
}
