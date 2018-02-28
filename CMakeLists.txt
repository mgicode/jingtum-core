# Project name
project(skywelld)

# Minium cmake requirement
cmake_minimum_required(VERSION 2.8)

#Set output directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add cpplags compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

# Add debug and release flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -Wall")

# platform 
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
else()
    message("\n W : *** linux only! *** \n")
endif()

#gcc > 4.8
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8)
else()
    message("\n GCC version 4.8.1 or later is required to compile skywelld. \n")
endif()

# Include headers
include_directories(src/common/beast)
include_directories(src)
include_directories(/usr/include/mysql)

# Add source dir
add_subdirectory(src)


