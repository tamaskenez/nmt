file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.cpp *.h)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(nmt0 ${sources})
target_link_libraries(nmt0 PRIVATE
	absl::flags
	absl::log
	absl::log_initialize
	fmt::fmt
	CLI11::CLI11
)

target_precompile_headers(nmt0 PRIVATE pch.h)
target_include_directories(nmt0 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS nmt0
	RUNTIME DESTINATION bin
)

