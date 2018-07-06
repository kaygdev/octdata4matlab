#pragma once


#include "matlab_types.h"
#include "mex.h"

#include <vector>
#include <tuple>
#include<type_traits>



template<typename T>
void transposeMatlabMatrix(mxArray* matlabMat)
{
	if(!matlabMat)
	{
		mexPrintf("matlabMat == nullptr\n");
		return;
	}

	if(mxGetClassID(matlabMat) != MatlabType<T>::classID)
	{
		mexPrintf("Transpose Matrix: Wrong ClassID: %d != %d\n", mxGetClassID(matlabMat), MatlabType<T>::classID);
		return;
	}

	if(mxGetNumberOfDimensions(matlabMat) != 2)
	{
		mexPrintf("Transpose Matrix: not 2D matrix : %d\n", mxGetNumberOfDimensions(matlabMat));
		return;
	}

	const mwSize m = mxGetM(matlabMat);
	const mwSize n = mxGetN(matlabMat);


	if(m != 1 && n != 1)
	{
		const T* const dataSource = reinterpret_cast<T*>(mxGetData(matlabMat));
		      T* const dataDest   = reinterpret_cast<T*>(mxCalloc(m*n, sizeof(T)));

		const T* sourceIt = dataSource;

			for(mwSize j = 0; j < n; ++j)
		{
		for(mwSize i = 0; i < m; ++i)
			{
				dataDest[i*n + j] = *sourceIt;
				++sourceIt;
			}
		}

		mxSetData(matlabMat, dataDest);
		mxFree(const_cast<T*>(dataSource));
	}

	mxSetM(matlabMat, n);
	mxSetN(matlabMat, m);
}


template<typename T>
void createMatlabVector(const std::vector<T>& vec, mxArray*& matlabMat)
{
	if(matlabMat)
		return;

	mwSize size = static_cast<mwSize>(vec.size());
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
void createMatlabVector(const std::vector<std::vector<T>>& vector, mxArray*& matlabMat)
{
	if(matlabMat)
		return;

	const mwSize length = static_cast<mwSize>(vector.size());
	matlabMat = mxCreateCellMatrix(1, length);
	mwSize index = 0;
	for(const std::vector<T>& ele : vector)
	{
		mxArray* cellEle = nullptr;
		createMatlabVector(ele, cellEle);
		mxSetCell(matlabMat, index, cellEle);
		++index;
	}
}

template<typename T>
void createMatlabArray(const T* vec, mxArray*& matlabMat, mwSize size)
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
mxArray* createMatlabArray(const T* vec, mwSize size)
{
	mxArray* matlabMat = nullptr;
	createMatlabArray<T>(vec, matlabMat, size);
	return matlabMat;
}


template<typename T>
void copyVec2MatlabRow(const std::vector<T>& vec, mwSize row, mxArray* matlabMat)
{
	if(!matlabMat)
	{
		mexPrintf("matlabMat == nullptr\n");
		return;
	}

	if(mxGetClassID(matlabMat) != MatlabType<T>::classID)
	{
		mexPrintf("Wrong ClassID: %d != %d\n", mxGetClassID(matlabMat), MatlabType<T>::classID);
		return;
	}

	T* matlabPtr = reinterpret_cast<T*>(mxGetPr(matlabMat));
	std::size_t cols = mxGetN(matlabMat);
	std::size_t rows = mxGetM(matlabMat);

	if(cols < vec.size())
	{
		mexPrintf("Matrix zu klein:: %d < %d\n", cols, vec.size());
		return;
	}

	matlabPtr += row;
	for(T value : vec)
	{
		*matlabPtr = value;
		matlabPtr += rows;
	}
}

template<typename T>
void copyVec2MatlabCol(const std::vector<T>& vec, std::size_t col, mxArray* matlabMat)
{
	if(!matlabMat)
	{
		mexPrintf("matlabMat == nullptr\n");
		return;
	}

	if(mxGetClassID(matlabMat) != MatlabType<T>::classID)
	{
		mexPrintf("Wrong ClassID: %d != %d\n", mxGetClassID(matlabMat), MatlabType<T>::classID);
		return;
	}

	T* matlabPtr = reinterpret_cast<T*>(mxGetPr(matlabMat));
	std::size_t cols = mxGetN(matlabMat);
	std::size_t rows = mxGetM(matlabMat);

	if(rows < vec.size())
	{
		mexPrintf("Matrix zu klein:: %d < %d\n", cols, vec.size());
		return;
	}

	matlabPtr += col*rows;
	for(T value : vec)
	{
		*matlabPtr = value;
		++matlabPtr;
	}
}

inline std::tuple<mwSize, mwSize> getMatrixSize(const mxArray* matlabMat)
{
	return std::make_tuple(mxGetN(matlabMat), mxGetM(matlabMat));
}


template<typename MatType, typename T>
void convertValue(T& ref, const mxArray* const matlabMat, const std::size_t eleNr = 0)
{
	void* dataPtr = mxGetData(matlabMat);
	ref = static_cast<T>(*(reinterpret_cast<const MatType*>(dataPtr)+eleNr));
}

template<typename MatType, typename T>
void convertValue(std::vector<T>& ref, const mxArray* const matlabMat, const std::size_t /*eleNr*/ = 0)
{
	void* dataPtr = mxGetData(matlabMat);
	const std::size_t size = mxGetM(matlabMat)*mxGetN(matlabMat);
	ref.resize(size);
	const MatType* in = reinterpret_cast<const MatType*>(dataPtr);

	for(std::size_t i = 0; i < size; ++i)
		ref[i] = static_cast<T>(in[i]);
}


template<typename T>
T getValueConvert(const mxArray* const matlabMat, const std::size_t eleNr = 0)
{
	if(!matlabMat)
		return T();

	switch(mxGetClassID(matlabMat))
	{
// #define HANDLE_TYPE(TYPE) case MatlabType<TYPE>::classID: return static_cast<T>(*(reinterpret_cast<const TYPE*>(dataPtr)+eleNr));
#define HANDLE_TYPE(TYPE) case MatlabType<TYPE>::classID: { T result; convertValue<TYPE>(result, matlabMat, eleNr); return result; };
		HANDLE_TYPE(double);
		HANDLE_TYPE(float);
		HANDLE_TYPE(bool);
		HANDLE_TYPE(uint8_t);
		HANDLE_TYPE(uint16_t);
		HANDLE_TYPE(uint32_t);
		HANDLE_TYPE(uint64_t);
		HANDLE_TYPE( int8_t);
		HANDLE_TYPE( int16_t);
		HANDLE_TYPE( int32_t);
		HANDLE_TYPE( int64_t);
#undef HANDLE_TYPE
		default:
			mexPrintf("unhandled Type: %d", mxGetClassID(matlabMat));
	}

	return T();
}

template<typename T>
T getScalarConvert(const mxArray* matlabMat)
{
	return getValueConvert<T>(matlabMat, 0);
}

template<>
std::string getScalarConvert(const mxArray* matlabMat)
{
	if(!matlabMat && mxGetClassID(matlabMat) !=  MatlabType<char>::classID)
	{
		mexPrintf("get String: wrong class %d != %d", mxGetClassID(matlabMat), MatlabType<char>::classID);
		return std::string();
	}

	const mxChar* fnPtr = reinterpret_cast<const mxChar*>(mxGetPr(matlabMat));
	std::size_t fnLength = mxGetN(matlabMat);
	std::string filename(fnPtr, fnPtr+fnLength);
	return filename;
}

template<typename T>
inline T getConfigFromStruct(const mxArray* const mxConfig, const char* name, const T defaultValue)
{
	mxArray* mxOpt = mxGetField(mxConfig, 0, name);
	if(mxOpt)
		return getScalarConvert<T>(mxOpt);

	return defaultValue;
}


class ParameterFromOptions
{
	const mxArray* mxOptions;
public:
	ParameterFromOptions(const mxArray* mxOptions) : mxOptions(mxOptions) {}

	template<typename T>
	void operator()(const char* name, T& value) { if(mxOptions) value = getConfigFromStruct(mxOptions, name, value); }

	ParameterFromOptions subSet(const std::string& name)
	{
		if(mxOptions)
			return ParameterFromOptions(mxGetField(mxOptions, 0, name.c_str()));
		return ParameterFromOptions(nullptr);
	}
};


class ParameterToOptions
{
	typedef std::vector<mxArray*> MxValueList;

	ParameterToOptions* parent = nullptr;
	std::size_t parent_num;

	MxValueList           valueList;
	std::vector<std::string> nameList;
public:
	~ParameterToOptions()
	{
		if(parent)
			parent->valueList[parent_num] = getMxOptions();

		for(std::size_t i = 0; i<valueList.size(); ++i)
			mxDestroyArray(valueList[i]);
	}

	template<typename T>
	void operator()(const std::string& name, T& value)
	{
		typedef typename std::remove_const<T>::type T_NOCONST;
		nameList.push_back(name);
		valueList.emplace_back(createMatlabArray<T_NOCONST>(&value, 1));
	}


	template<typename T>
	void operator()(const std::string& name, const std::vector<T>& value)
	{
		nameList.push_back(name);
		mxArray* mat = nullptr;
		createMatlabVector(value, mat);
		valueList.push_back(mat);
	}


	ParameterToOptions subSet(const std::string& name)
	{
		ParameterToOptions pto;

		nameList.push_back(name);
		valueList.push_back(nullptr);

		pto.parent = this;
		pto.parent_num = valueList.size() - 1;

		return pto;
	}

	void addMxArray(const std::string& name, mxArray* array)
	{
		if(array)
		{
			nameList.push_back(name);
			valueList.push_back(array);
		}
	}

	mxArray* getMxOptions()
	{
		if(valueList.empty())
			return nullptr;

		std::vector<const char*> nameListCstr;
		nameListCstr.reserve(nameList.size());
		for(const std::string& name : nameList)
			nameListCstr.push_back(name.data());

		mxArray* mxOptions = mxCreateStructMatrix(1, 1, static_cast<int>(nameListCstr.size()), nameListCstr.data());

		for(std::size_t i = 0; i<valueList.size(); ++i)
			mxSetFieldByNumber(mxOptions, 0, static_cast<int>(i), valueList[i]);

		nameList.clear();
		valueList.clear();

		return mxOptions;
	}
};

template<>
void ParameterToOptions::operator()(const std::string& name, const std::string& value)
{
	nameList.push_back(name);
	valueList.emplace_back(mxCreateString(value.c_str()));
}

template<>
void ParameterToOptions::operator()(const std::string& name, std::string& value)
{
	nameList.push_back(name);
	valueList.emplace_back(mxCreateString(value.c_str()));
}
