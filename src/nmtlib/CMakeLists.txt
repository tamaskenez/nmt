if(NOT TARGET libtokenizer::libtokenizer)
	find_package(libtokenizer REQUIRED)
endif()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.cpp *.h)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_library(nmtlib STATIC ${sources})
target_link_libraries(nmtlib
	PRIVATE
		absl::log
		absl::log_initialize
		libtokenizer::libtokenizer
		CLI11::CLI11
	PUBLIC
		util
		absl::flat_hash_map
		fmt::fmt
)

target_precompile_headers(nmtlib PRIVATE pch.h)
target_include_directories(nmtlib
	PRIVATE
		.
	PUBLIC
		public
)


