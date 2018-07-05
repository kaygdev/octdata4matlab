#pragma once

#include <opencv/cv.h>

#include "matlab_types.h"
#include "mex.h"


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
mxArray* convertMatrix(const cv::Mat& cvMat)
{
	mxArray* matlabMat = nullptr;
	createCopyMatrix<T>(cvMat, matlabMat);
	return matlabMat;
}
