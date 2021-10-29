cmake_minimum_required(VERSION 3.10)

project(lajolla)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(lajolla main.cpp image.cpp)