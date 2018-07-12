cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(octdata2matlab)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_module/)

find_package(LibOctData REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost 1.40 REQUIRED)

find_package(Matlab COMPONENTS MX_LIBRARY)
find_package(Octave COMPONENTS MX_LIBRARY)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wunreachable-code -Wconversion")
# 	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")
endif()

include_directories(SYSTEM ${Boost_INCLUDE_DIR})
include_directories(${LibOctData_INCLUDE_DIRS})

# message(${Matlab_INCLUDE_DIRS})
# message(${Matlab_LIBRARIES})

if(Matlab_FOUND)
	matlab_add_mex(NAME readoctdata  SRC readoctdata.cpp  LINK_TO ${LibOctData_LIBRARIES} ${OpenCV_LIBRARIES})
	matlab_add_mex(NAME writeoctdata SRC writeoctdata.cpp LINK_TO ${LibOctData_LIBRARIES} ${OpenCV_LIBRARIES})

	target_include_directories(readoctdata  SYSTEM PRIVATE ${Matlab_INCLUDE_DIRS} PUBLIC ${CMAKE_SOURCE_DIR}/src/)
	target_include_directories(writeoctdata SYSTEM PRIVATE ${Matlab_INCLUDE_DIRS} PUBLIC ${CMAKE_SOURCE_DIR}/src/)
endif()

if(OCTAVE_LIBRARIES)
	octave_add_oct(readoctdata_octave  SOURCES readoctdata.cpp  LINK_LIBRARIES ${LibOctData_LIBRARIES} ${OpenCV_LIBRARIES} EXTENSION mex)
	octave_add_oct(writeoctdata_octave SOURCES writeoctdata.cpp LINK_LIBRARIES ${LibOctData_LIBRARIES} ${OpenCV_LIBRARIES} EXTENSION mex)

	target_include_directories(readoctdata_octave  SYSTEM PRIVATE ${OCTAVE_INCLUDE_DIRS} PUBLIC ${CMAKE_SOURCE_DIR}/src/)
	target_include_directories(writeoctdata_octave SYSTEM PRIVATE ${OCTAVE_INCLUDE_DIRS} PUBLIC ${CMAKE_SOURCE_DIR}/src/)

	set_target_properties(readoctdata_octave  PROPERTIES OUTPUT_NAME "readoctdata" )
	set_target_properties(writeoctdata_octave PROPERTIES OUTPUT_NAME "writeoctdata")
endif()
