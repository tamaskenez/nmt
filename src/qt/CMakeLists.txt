find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

add_subdirectory(qtui) # uses common
add_subdirectory(nmtqt) # uses qtui, nmtapp, common
