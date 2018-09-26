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

#pragma once

#include <opencv/cv.h>

#include "matlab_types.h"
#include "mex.h"


template<typename T>
void copyMatrixTranspose(const cv::Mat& cvMat, T* matlabPtr, std::size_t channel)
{
	const int sizeCols = cvMat.cols;
	const int sizeRows = cvMat.rows;
	const int channels = cvMat.channels();

	for(int i = 0; i < sizeRows; ++i)
	{
		T* matlabLine = matlabPtr + i;
		const T* ptr = cvMat.ptr<T>(i) + channel;
		for(int j = 0; j < sizeCols; ++j)
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
	if(channels == 3) // convert opencv bgr to rgb
	{
		copyMatrixTranspose(cvMat, matlabPtr + 0*sizeCols*sizeRows, 2);
		copyMatrixTranspose(cvMat, matlabPtr + 1*sizeCols*sizeRows, 1);
		copyMatrixTranspose(cvMat, matlabPtr + 2*sizeCols*sizeRows, 0);
	}
	else
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
			copyMatrixTranspose(cvMat, matlabPtr + channel*sizeCols*sizeRows, channel);
	}
}

template<typename T>
void createCopyMatrix(const cv::Mat& cvMat, mxArray*& matlabMat)
{
	if(matlabMat)
		return;

	const mwSize sizeCols = static_cast<mwSize>(cvMat.cols      );
	const mwSize sizeRows = static_cast<mwSize>(cvMat.rows      );
	const mwSize channels = static_cast<mwSize>(cvMat.channels());

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
	const int sizeCols = cvMat.cols;
	const int sizeRows = cvMat.rows;
	const int channels = cvMat.channels();

	for(int i = 0; i < sizeRows; ++i)
	{
		const T* matlabLine = matlabPtr + i;
		T* ptr = cvMat.ptr<T>(i) + channel;
		for(int j = 0; j < sizeCols; ++j)
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

	if(mxGetClassID(matlabMat) != MatlabType<T>::classID)
	{
		mexPrintf("copyMatrix: Wrong ClassID: %d != %d\n", mxGetClassID(matlabMat), MatlabType<T>::classID);
		return;
	}

	const std::size_t sizeCols = cvMat.cols;
	const std::size_t sizeRows = cvMat.rows;
	const std::size_t channels = cvMat.channels();

	const T* matlabPtr = reinterpret_cast<const T*>(mxGetPr(matlabMat));
	// copy transpose matrix because opencv's structure is row based and matlab's structure is col based
	if(channels == 3) // convert opencv bgr to rgb
	{
		copyMatrixTranspose(matlabPtr + 0*sizeCols*sizeRows, cvMat, 2);
		copyMatrixTranspose(matlabPtr + 1*sizeCols*sizeRows, cvMat, 1);
		copyMatrixTranspose(matlabPtr + 2*sizeCols*sizeRows, cvMat, 0);
	}
	else
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		copyMatrixTranspose(matlabPtr + channel*sizeCols*sizeRows, cvMat, channel);
	}
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

	int channels = numDims==3?static_cast<int>(dims[2]):1;

	cv::Mat cvMat(static_cast<int>(dims[0]), static_cast<int>(dims[1]), CV_MAKETYPE(cv::DataType<uint8_t>::type, channels));
	copyMatrix<T>(matlabMat, cvMat);
	return cvMat;
}
