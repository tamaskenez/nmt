file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.cpp *.h)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(nmt0 ${sources})
target_link_libraries(nmt0 PRIVATE
	fmt::fmt
	glog::glog
	nonstd::expected-lite
)

install(TARGETS nmt0
	RUNTIME DESTINATION bin
)

