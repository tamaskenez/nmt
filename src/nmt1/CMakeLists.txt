file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/nmt0/src/*.h)
source_group(TREE ${CMAKE_SOURCE_DIR}/nmt0/src FILES ${sources})
add_executable(nmt1 ${sources})

run_nmt(nmt1 FILES ${sources})

target_link_libraries(nmt1 PRIVATE
	fmt::fmt
	glog::glog
	nonstd::expected-lite
)
install(TARGETS nmt1
	RUNTIME DESTINATION bin
)

