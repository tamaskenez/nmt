function(run_nmt target)
	cmake_parse_arguments(PARSE_ARGV 1 ARG
		""
		"SOURCE_DIR"
		"")
	if(NOT ARG_SOURCE_DIR)
		message(FATAL_ERROR "Missing parameter: SOURCE_DIR")
	endif()

	# Default base directory is CMAKE_CURRENT_SOURCE_DIR.
	cmake_path(ABSOLUTE_PATH ARG_SOURCE_DIR NORMALIZE)

	# Perform GLOB_RECURSE to trigger config if anything changes
	# in the source directory.
	file(GLOB_RECURSE sources CONFIGURE_DEPENDS *)

	set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/nmt")
	execute_process(COMMAND ${NMT_PROGRAM}
		--source-dir ${ARG_SOURCE_DIR}
		--target ${target}
		--output-dir ${output_dir}
		COMMAND_ECHO STDOUT
		COMMAND_ERROR_IS_FATAL ANY
	)

	# Read output file list from running `nmt`.
	file(STRINGS ${output_dir}/files.txt generated_files)

	# Diagnostics; print the contents of `files.txt`
	set(generated_files_rel "")
	foreach(f IN LISTS generated_files)
 		cmake_path(RELATIVE_PATH f BASE_DIRECTORY ${output_dir})
 		set(generated_files_rel "${generated_files_rel}${f} ")
 	endforeach()
	message(STATUS "reading file list ${output_dir}/files.txt -> ${generated_files_rel}")

	# Add the file list and the generated source files to the target.
	source_group(boilerplate FILES ${output_dir}/files.txt ${generated_files})
	target_sources(${target} PRIVATE ${output_dir}/files.txt ${generated_files})
	target_include_directories(${target}
		PUBLIC ${output_dir}/public
		PRIVATE ${output_dir}/private
	)

	add_custom_command(TARGET ${target}
	                   PRE_BUILD
	                   COMMAND ${NMT_PROGRAM}
	                   --source-dir ${ARG_SOURCE_DIR}
	                   --target ${target}
	                   --output-dir ${output_dir}
	                   BYPRODUCTS ${output_dir}/files.txt
	                   COMMENT "Running nmt."
	)
endfunction()
