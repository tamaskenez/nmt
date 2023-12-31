file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources}})
add_library(nmtapplogic STATIC ${sources})
run_nmt(nmtapplogic SOURCE_DIR "#")

target_link_libraries(nmtapplogic PRIVATE
	appcommon
	nmtlib
	fmt::fmt
)

target_include_directories(nmtapplogic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
