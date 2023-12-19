find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

add_subdirectory(qtui) # uses common
add_subdirectory(nmtqtapp) # uses qtui, nmtapplogic, common
