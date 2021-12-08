cmake_minimum_required(VERSION 3.10)

project(lajolla)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Embree REQUIRED)
include_directories(${EMBREE_INCLUDE_PATH})
link_libraries(${EMBREE_LIBRARY})

add_executable(lajolla main.cpp camera.cpp image.cpp transform.cpp)