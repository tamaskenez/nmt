cmake_minimum_required(VERSION 3.25)
project(nmt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 1)

find_package(fmt REQUIRED)
find_package(glog REQUIRED)

add_subdirectory(src)
