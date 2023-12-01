add_subdirectory(util)

add_subdirectory(nmt)

if(BUILD_FULL)
	add_subdirectory(helloqt)
	add_subdirectory(common)
	add_subdirectory(nmtapp) # uses common
	add_subdirectory(qt)
endif()
