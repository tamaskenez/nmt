cmake_minimum_required(VERSION 3.25)
project(nmt VERSION 0.2)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

message(STATUS "BUILD_TESTING: ${BUILD_TESTING}")

include(cmake/RunNMT.cmake)

if(BUILD_TESTING)
	enable_testing()
	find_package(GTest REQUIRED CONFIG)
endif()

find_package(absl REQUIRED)
find_package(CLI11 REQUIRED)
find_package(fmt REQUIRED)

# Adding tokenizer before setting strict warning options.
add_subdirectory(thirdparty/dspinellis_tokenizer)

set(WARNINGS_AS_ERRORS 1)
include(cmake/cpp_warnings.cmake)
add_compile_options(${PROJECT_WARNING_FLAGS})

if(BUILD_FULL)
	set(nmt_only_build_dir ${CMAKE_CURRENT_BINARY_DIR}/nmt_only)
	execute_process(
		COMMAND ${CMAKE_COMMAND}
			-S ${CMAKE_CURRENT_SOURCE_DIR}
			-B ${nmt_only_build_dir}
			-D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
			-G ${CMAKE_GENERATOR}
			-D CMAKE_BUILD_TYPE=Debug
			-D BUILD_FULL=0
			-D BUILD_TESTING=0
			-D CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
		COMMAND_ECHO STDOUT
		COMMAND_ERROR_IS_FATAL ANY
	)
	execute_process(
		COMMAND ${CMAKE_COMMAND}
			--build ${nmt_only_build_dir}
			--target install
			--config Debug
			-j
		COMMAND_ECHO STDOUT
		COMMAND_ERROR_IS_FATAL ANY
	)
	find_program(NMT_PROGRAM nmt
		PATHS ${CMAKE_INSTALL_PREFIX}/bin
		NO_DEFAULT_PATH
		REQUIRED
	)
endif()

add_subdirectory(src)

if(BUILD_TESTING)
	add_subdirectory(test)
endif()
