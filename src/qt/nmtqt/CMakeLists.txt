file(GLOB sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

qt_add_executable(nmtqt
	WIN32 MACOSX_BUNDLE
  ${sources}
)

run_nmt(nmtqt FILES ${sources})

set_target_properties(nmtqt PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER nmtqt.tamas.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

target_link_libraries(nmtqt PRIVATE
    common
    qtui
    nmtapp
)
