cmake_minimum_required(VERSION 3.0.2)

project(XorList LANGUAGES CXX)

add_library(XorList INTERFACE)
target_include_directories(XorList INTERFACE include/)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(tests)
