file(GLOB_RECURSE sourcese CONFIGURE_DEPENDS "#/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sourcese} ${sources}})
add_library(nmtapp STATIC ${sourcese} ${sources})
run_nmt(nmtapp FILES ${sourcese})

target_link_libraries(nmtapp PRIVATE
	common
)

target_include_directories(nmtapp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
