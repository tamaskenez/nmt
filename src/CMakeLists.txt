file(GLOB_RECURSE sources *.cpp *.h)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(nmt ${sources})
target_link_libraries(nmt PRIVATE
	fmt::fmt
	glog::glog
)
