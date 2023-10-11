
if(NOT USE_INSTALLED_TOOL)
	file(GLOB_RECURSE sources *.cpp *.h)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

	add_executable(nmt ${sources})
	target_link_libraries(nmt PRIVATE
		fmt::fmt
		glog::glog
		nonstd::expected-lite
	)
	install(TARGETS nmt
		RUNTIME DESTINATION bin
	)
else()
	find_program(NMT_PROGRAM nmt REQUIRED
		NO_PACKAGE_ROOT_PATH
		NO_CMAKE_ENVIRONMENT_PATH
		NO_SYSTEM_ENVIRONMENT_PATH
		NO_CMAKE_SYSTEM_PATH
	)
	message(STATUS "NMT_PROGRAM: ${NMT_PROGRAM}")
	file(GLOB_RECURSE sources s/*.h)
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
	cmake_path(CONVERT "${sources}" TO_NATIVE_PATH_LIST native_sources NORMALIZE)
	set(sources_input_file_path "${CMAKE_CURRENT_BINARY_DIR}/generated/sources_input.txt")
	set(sources_output_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/sources_output")
	list(JOIN sources "\n" content)
	file(WRITE ${sources_input_file_path} ${content} "\n")
endif()

