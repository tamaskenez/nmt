cmake_minimum_required(VERSION 3.25)
project(nmt0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 1)

find_package(fmt REQUIRED)
find_package(glog REQUIRED)
find_package(expected-lite REQUIRED)

set(WARNINGS_AS_ERRORS 1)
include(${CMAKE_CURRENT_SOURCE_DIR}/../build/cpp_warnings.cmake)
add_compile_options(${PROJECT_WARNING_FLAGS})

add_subdirectory(src)
