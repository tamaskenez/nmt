cmake_minimum_required(VERSION 3.25)
project(nmt VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

message(STATUS "BUILD_TESTING: ${BUILD_TESTING}")

include(build/RunNMT.cmake)

if(BUILD_TESTING)
	enable_testing()
	find_package(GTest REQUIRED CONFIG)
endif()


find_package(absl REQUIRED)
find_package(CLI11 REQUIRED)
find_package(fmt REQUIRED)

set(WARNINGS_AS_ERRORS 1)
include(build/cpp_warnings.cmake)
add_compile_options(${PROJECT_WARNING_FLAGS})

if(BUILD_ALL)
	find_program(NMT_PROGRAM nmt
		PATHS ${CMAKE_PREFIX_PATH}
		NO_DEFAULT_PATH
	)
	if(NMT_PROGRAM)
		message(STATUS "NMT_PROGRAM: ${NMT_PROGRAM}")
	else()
		message(FATAL_ERROR "`nmt` program not found. Run nmt0/2_make_install.sh to build it.")
	endif()
endif()

add_subdirectory(src)

if(BUILD_TESTING)
	add_subdirectory(test)
endif()

