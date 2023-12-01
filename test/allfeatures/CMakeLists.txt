file(GLOB_RECURSE nmt_sources CONFIGURE_DEPENDS "#/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(allfeatures_test ${sources})

run_nmt(allfeatures_test FILES ${nmt_sources})
