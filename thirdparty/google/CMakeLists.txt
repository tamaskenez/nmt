cmake_minimum_required(VERSION 3.25)
project(google)

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/ABSL/s ABSL/b)
#add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/RE2/s RE2/b)
#add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/GTEST/s GTEST/b)
