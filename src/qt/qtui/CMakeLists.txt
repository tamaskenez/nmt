file(GLOB sources CONFIGURE_DEPENDS *.h)
file(GLOB sourcespp CONFIGURE_DEPENDS *.hpp *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources} ${sourcespp})

qt_add_library(qtui ${sources} ${sourcespp})

run_nmt(qtui FILES ${sources})

target_link_libraries(qtui PRIVATE
	common
	Qt6::Widgets
	Qt6::Gui
)

target_include_directories(qtui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
