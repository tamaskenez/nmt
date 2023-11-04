file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
add_library(util STATIC ${sources})
target_include_directories(util
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(util PUBLIC absl::log)