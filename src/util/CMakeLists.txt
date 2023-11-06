file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
list(FILTER sources EXCLUDE REGEX ".*_test[.]cpp")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})
add_library(util STATIC ${sources})
target_include_directories(util
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(util PUBLIC absl::log)

if(BUILD_TESTING)

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS *_test.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${test_sources})
add_executable(test_util ${test_sources})
target_link_libraries(test_util PRIVATE
	util
	GTest::gtest
	GTest::gtest_main
)
add_test(NAME test_util COMMAND test_util)

endif()