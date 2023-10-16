file(GLOB sources CONFIGURE_DEPENDS *.h)
file(GLOB sourcespp CONFIGURE_DEPENDS *.hpp *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources} ${sourcespp}})
add_library(nmtapp STATIC ${sources} ${sourcespp})
run_nmt(nmtapp FILES ${sources})

target_link_libraries(nmtapp PRIVATE
	common
)

target_include_directories(nmtapp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
