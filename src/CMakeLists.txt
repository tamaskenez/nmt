add_subdirectory(util)

add_subdirectory(nmtlib)
add_subdirectory(nmt)

if(BUILD_FULL)
	add_subdirectory(helloqt)
	add_subdirectory(common)
	add_subdirectory(nmtapplogic) # uses common
	add_subdirectory(qt)
endif()
