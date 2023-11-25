function(run_nmt target)
	cmake_parse_arguments(PARSE_ARGV 1 ARG
		""
		""
		"FILES")
	set(sources_input_file_path "${CMAKE_CURRENT_BINARY_DIR}/generated/sources_input.txt")
	set(sources_output_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/sources_output")
	list(JOIN ARG_FILES "\n" content)
	file(WRITE ${sources_input_file_path} ${content} "\n")
	execute_process(COMMAND ${NMT_PROGRAM}
		--sources-files ${sources_input_file_path}
		--output-dir ${sources_output_dir}
		COMMAND_ERROR_IS_FATAL ANY
	)
	file(STRINGS ${sources_output_dir}/files.txt generated_files)
	message(STATUS "reading file list ${sources_output_dir}/files.txt -> ${generated_files}")
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