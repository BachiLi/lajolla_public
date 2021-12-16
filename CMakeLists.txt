cmake_minimum_required(VERSION 3.10)

project(lajolla)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

find_package(Embree REQUIRED)
include_directories(${EMBREE_INCLUDE_PATH})
link_libraries(${EMBREE_LIBRARY})

set(SRCS camera.h
         flexception.h
         frame.h
         image.h
         intersection.h
         lajolla.h
         light.h
         load_serialized.h
         material.h
         matrix.h
         miniz.h
         mipmap.h
         parse_obj.h
         parse_scene.h
         point_and_normal.h
         pugiconfig.hpp
         pugixml.hpp
         ray.h
         render.h
         scene.h
         shape.h
         spectrum.h
         stb_image.h
         table_dist.h
         texture.h
         transform.h
         vector.h
         camera.cpp
         image.cpp
         intersection.cpp
         light.cpp
         load_serialized.cpp
         main.cpp
         material.cpp
         miniz.c
         parse_obj.cpp
         parse_scene.cpp
         pugixml.cpp
         render.cpp
         scene.cpp
         shape.cpp
         table_dist.cpp
         transform.cpp)

add_executable(lajolla ${SRCS})