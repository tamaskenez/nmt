file(GLOB sourcese CONFIGURE_DEPENDS "#/*.h")
file(GLOB sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sourcese} ${sources})

qt_add_library(qtui ${sourcese} ${sources})

run_nmt(qtui FILES ${sourcese})

target_link_libraries(qtui PRIVATE
	common
	Qt6::Widgets
	Qt6::Gui
)

target_include_directories(qtui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
