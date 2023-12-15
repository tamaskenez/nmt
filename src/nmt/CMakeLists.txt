file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.cpp *.h)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(nmt ${sources})
target_link_libraries(nmt PRIVATE
	nmtlib
	#util
	#
	#absl::log
	#absl::log_initialize
	#absl::flat_hash_map
	#fmt::fmt
	CLI11::CLI11
)

target_precompile_headers(nmt PRIVATE pch.h)

install(TARGETS nmt RUNTIME DESTINATION bin)
