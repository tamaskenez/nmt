cmake_minimum_required(VERSION 3.25)
project(nmt VERSION 0.1)

message(STATUS "CMAKE_PREFIX_PATH:")
foreach(i IN LISTS CMAKE_PREFIX_PATH)
	message(STATUS "${i}")
endforeach()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 1)

find_package(fmt REQUIRED)
find_package(glog REQUIRED)
find_package(expected-lite REQUIRED)

set(WARNINGS_AS_ERRORS 1)
include(build/cpp_warnings.cmake)
add_compile_options(${PROJECT_WARNING_FLAGS})

find_program(NMT0_PROGRAM nmt0
	PATHS ${CMAKE_SOURCE_DIR}/nmt0/o/bin
	NO_DEFAULT_PATH
)
if(NMT0_PROGRAM)
	message(STATUS "NMT0_PROGRAM: ${NMT0_PROGRAM}")
else()
	message(FATAL_ERROR "`nmt0` program not found. Run nmt0/2_make_install.sh to build it.")
endif()

function(run_nmt target)
	cmake_parse_arguments(PARSE_ARGV 1 ARG
		""
		""
		"FILES")
	set(sources_input_file_path "${CMAKE_CURRENT_BINARY_DIR}/generated/sources_input.txt")
	set(sources_output_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/sources_output")
	list(JOIN ARG_FILES "\n" content)
	file(WRITE ${sources_input_file_path} ${content} "\n")
	execute_process(COMMAND ${NMT0_PROGRAM}
		--sources-files ${sources_input_file_path}
		--output-dir ${sources_output_dir}
		COMMAND_ERROR_IS_FATAL ANY
	)
	file(STRINGS ${sources_output_dir}/files.txt generated_files)
	source_group(boilerplate FILES ${sources_output_dir}/files.txt ${generated_files})
	target_sources(${target} PRIVATE ${sources_output_dir}/files.txt ${generated_files})
	target_include_directories(${target} PUBLIC ${sources_output_dir})

	add_custom_command(TARGET ${target}
	                   PRE_BUILD
	                   COMMAND ${NMT0_PROGRAM}
	            	   		--sources-files ${sources_input_file_path}
							--output-dir ${sources_output_dir}
	                   BYPRODUCTS ${sources_output_dir}/files.txt
	                   COMMENT "Running nmt."
	)

endfunction()

add_subdirectory(src)
