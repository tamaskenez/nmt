add_subdirectory(util)

add_subdirectory(nmtlib)
add_subdirectory(nmt)

if(BUILD_FULL)
	add_subdirectory(helloqt)
	add_subdirectory(appcommon)
	add_subdirectory(nmtapplogic) # uses appcommon
	add_subdirectory(qt)
endif()
