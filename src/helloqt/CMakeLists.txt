find_package(Qt6 REQUIRED COMPONENTS Core Quick Widgets CONFIG)
qt_standard_project_setup()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_AUTORCC ON)

file(GLOB sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

qt_add_executable(helloqt
	WIN32 MACOSX_BUNDLE
    ${sources}
)

target_compile_definitions(helloqt
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(helloqt
  PRIVATE Qt6::Widgets Qt6::Quick)

set_target_properties(helloqt PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER helloqt.tamas.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)
