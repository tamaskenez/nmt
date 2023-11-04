add_subdirectory(util)

if(BUILD_NMT)
	add_subdirectory(nmt)
endif()

if(BUILD_ALL)
	add_subdirectory(helloqt)
	add_subdirectory(common)
	add_subdirectory(nmtapp) # uses common
	add_subdirectory(qt)
endif()
