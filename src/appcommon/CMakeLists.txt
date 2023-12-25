file(GLOB_RECURSE sourcese CONFIGURE_DEPENDS "#/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
add_library(appcommon STATIC ${sources})
run_nmt(appcommon FILES ${sourcese})
target_link_libraries(appcommon PUBLIC
    nmtlib
)
target_precompile_headers(appcommon PRIVATE pch.h)
