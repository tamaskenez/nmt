find_program(NMT0_PROGRAM nmt0 REQUIRED
	PATHS ${CMAKE_SOURCE_DIR}/nmt0/o/bin
	NO_DEFAULT_PATH
)
if(NMT0_PROGRAM)
	message(STATUS "NMT0_PROGRAM: ${NMT0_PROGRAM}")
else()
	message(FATAL_ERROR "`nmt0` program not found. Run nmt0/2_make_install.sh to build it.")
endif()

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/nmt0/src/*.h)
source_group(TREE ${CMAKE_SOURCE_DIR}/nmt0/src FILES ${sources})
set(sources_input_file_path "${CMAKE_CURRENT_BINARY_DIR}/generated/sources_input.txt")
set(sources_output_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/sources_output")
list(JOIN sources "\n" content)
file(WRITE ${sources_input_file_path} ${content} "\n")
execute_process(COMMAND ${NMT0_PROGRAM}
	--sources-files ${sources_input_file_path}
	--output-dir ${sources_output_dir}
	COMMAND_ERROR_IS_FATAL ANY
)
file(STRINGS ${sources_output_dir}/files.txt generated_files)
source_group(TREE ${sources_output_dir} FILES ${generated_files})
add_executable(nmt1 ${sources} ${generated_files})

target_link_libraries(nmt1 PRIVATE
	fmt::fmt
	glog::glog
	nonstd::expected-lite
)
install(TARGETS nmt1
	RUNTIME DESTINATION bin
)

