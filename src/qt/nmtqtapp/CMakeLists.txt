file(GLOB sourcese CONFIGURE_DEPENDS "#/*.h")
file(GLOB sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sourcese} ${sources})

qt_add_executable(nmtqtapp
	WIN32 MACOSX_BUNDLE
  ${sourcese} ${sources}
)

run_nmt(nmtqtapp FILES ${sourcese})

set_target_properties(nmtqtapp PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "nmt"
    MACOSX_BUNDLE_COPYRIGHT "© 2023 Tamas"
    #MACOSX_BUNDLE_EXECUTABLE_NAME
    #MACOSX_BUNDLE_ICON_FILE
    #MACOSX_BUNDLE_INFO_PLIST
    #MACOSX_PACKAGE_LOCATION
    #PRODUCT_NAME? # CFBundleName
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_GUI_IDENTIFIER nmtqtapp.tamas.com # CFBundleIdentifier
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    #XCODE_ATTRIBUTE_MARKETING_VERSION 1.2
    #XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION 3.4
    #XCODE_ATTRIBUTE_INFOPLIST_KEY_NSHumanReadableCopyright "the copyright 2"
)

target_link_libraries(nmtqtapp PRIVATE
    common
    qtui
    nmtapplogic
)


