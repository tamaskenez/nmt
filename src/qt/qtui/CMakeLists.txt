file(GLOB sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

qt_add_library(qtui ${sources})

run_nmt(qtui SOURCE_DIR "#")

target_link_libraries(qtui PRIVATE
	appcommon
	Qt6::Widgets
	Qt6::Gui
)

target_include_directories(qtui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(qtui PRIVATE pch.h)
