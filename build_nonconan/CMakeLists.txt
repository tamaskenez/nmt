cmake_minimum_required(VERSION 3.28)
project(build_nonconan)

include(ExternalProject)
ExternalProject_Add(absl
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp
    GIT_TAG        fb3621f4f897824c0dbe0615fa94543df6192f30  # 20230801.1
    CMAKE_ARGS     -D CMAKE_CXX_STANDARD=23
                   -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                   -D CMAKE_DEBUG_POSTFIX=_d
                   -D BUILD_TESTING=0
                   -D ABSL_PROPAGATE_CXX_STD=1
)
