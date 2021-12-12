cmake_minimum_required(VERSION 3.10)

project(lajolla)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Embree REQUIRED)
include_directories(${EMBREE_INCLUDE_PATH})
link_libraries(${EMBREE_LIBRARY})

set(SRCS camera.h
         image.h
         intersection.h
         lajolla.h
         light.h
         material.h
         matrix.h
         ray.h
         render.h
         scene.h
         shape.h
         transform.h
         vector.h
         camera.cpp
         image.cpp
         intersection.cpp
         light.cpp
         main.cpp
         material.cpp
         render.cpp
         scene.cpp
         shape.cpp
         transform.cpp)

add_executable(lajolla ${SRCS})