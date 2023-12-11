function(run_nmt target)
	cmake_parse_arguments(PARSE_ARGV 1 ARG
		""
		""
		"FILES")
	set(sources_input_file_path "${CMAKE_CURRENT_BINARY_DIR}/generated/sources_input.txt")
	set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/nmt")
	list(JOIN ARG_FILES "\n" content)
	file(WRITE ${sources_input_file_path} ${content} "\n")
	execute_process(COMMAND ${NMT_PROGRAM}
		--sources ${sources_input_file_path}
		--output-dir ${output_dir}
		COMMAND_ECHO STDOUT
		COMMAND_ERROR_IS_FATAL ANY
	)
	file(STRINGS ${output_dir}/files.txt generated_files)
	set(generated_files_rel "")
	foreach(f IN LISTS generated_files)
 		cmake_path(RELATIVE_PATH f BASE_DIRECTORY ${output_dir})
 		set(generated_files_rel "${generated_files_rel}${f} ")
 	endforeach()
	message(STATUS "reading file list ${output_dir}/files.txt -> ${generated_files_rel}")
	source_group(boilerplate FILES ${output_dir}/files.txt ${generated_files})
	target_sources(${target} PRIVATE ${output_dir}/files.txt ${generated_files})
	target_include_directories(${target}
		PUBLIC ${output_dir}/public
		PRIVATE ${output_dir}/private
	)

	add_custom_command(TARGET ${target}
	                   PRE_BUILD
	                   COMMAND ${NMT_PROGRAM}
	            	   		--sources ${sources_input_file_path}
							--output-dir ${output_dir}
	                   BYPRODUCTS ${output_dir}/files.txt
	                   COMMENT "Running nmt."
	)
endfunction()