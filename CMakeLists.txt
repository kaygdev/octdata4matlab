cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(octdata2matlab)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_module/)

find_package(LibOctData REQUIRED)
find_package(Matlab REQUIRED)
find_package(OpenCV REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(compiler_flags_debug "-Wall -Wextra -Wunreachable-code")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")
endif()

include_directories(${LibOctData_INCLUDE_DIRS})
include_directories(${Matlab_INCLUDE_DIRS})

message(${Matlab_INCLUDE_DIRS})
message(${Matlab_LIBRARIES})

matlab_add_mex(NAME octdata2matlab SRC main.cpp LINK_TO ${LibOctData_LIBRARIES})

target_link_libraries(octdata2matlab ${LibOctData_LIBRARIES} ${OpenCV_LIBRARIES} ${Matlab_LIBRARIES})

