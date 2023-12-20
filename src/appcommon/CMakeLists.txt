file(GLOB sourcese CONFIGURE_DEPENDS "#/*.h")
file(GLOB sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sourcese} ${sources})
add_library(appcommon STATIC ${sourcese} ${sources})
run_nmt(appcommon FILES ${sourcese})
target_link_libraries(appcommon PRIVATE
    nmtlib
)
target_precompile_headers(appcommon PRIVATE pch.h)
