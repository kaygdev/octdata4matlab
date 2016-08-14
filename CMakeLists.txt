cmake_minimum_required(VERSION 3.0)
project(octdata2matlab)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_module/)

find_package(LibOctData REQUIRED)
find_package(Matlab REQUIRED)
find_package(OpenCV REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX)
	set(compiler_flags_debug "-Wall -Wextra -Wunreachable-code")        ## Optimize -O3 -Weverything -Weffc++
	# set(compiler_flags_release "-std=c++11")        ## Optimize -O3 -Weverything
	set(CMAKE_CXX_FLAGS "-std=c++11 -static-libstdc++ -L/usr/lib/x86_64-linux-gnu/libstdc++.so.6")
endif()

include_directories(${LibOctData_INCLUDE_DIRS})

include_directories(${MATLAB_INCLUDE_DIR})

message(${MATLAB_INCLUDE_DIR})

matlab_add_mex(NAME octdata2matlab SRC main.cpp) # nicht verfügbar unter Cmake3.0
# add_executable(octdata2matlab main.cpp)

# add_library(octdata2matlab STATIC main.cpp)

# install(TARGETS octdata2matlab RUNTIME DESTINATION bin)

target_link_libraries(octdata2matlab ${LibOctData_LIBRARIES} ${OpenCV_LIBRARIES})

