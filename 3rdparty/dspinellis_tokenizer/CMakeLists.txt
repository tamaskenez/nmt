cmake_minimum_required(VERSION 3.25)
project(tokenizer)

# git and perl needed.
find_package(Perl REQUIRED)
find_package(Git REQUIRED)
macro(execute_perl)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${ARGV}
        COMMAND_ECHO STDOUT
        COMMAND_ERROR_IS_FATAL ANY)
endmacro()

# Clone github/dspinellis/tokenizer.
include(FetchContent)
FetchContent_Declare(tokenizer
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tokenizer
	GIT_REPOSITORY "https://github.com/dspinellis/tokenizer.git"
)
message(STATUS "FetchContent_MakeAvailable(tokenizer)...")
FetchContent_MakeAvailable(tokenizer)

set(src ${CMAKE_CURRENT_SOURCE_DIR}/tokenizer/src)

# Check if the original sources are already modified.
execute_process(COMMAND ${GIT_EXECUTABLE} diff origin/head --exit-code
	OUTPUT_QUIET
	RESULT_VARIABLE git_result
	WORKING_DIRECTORY tokenizer
)

if(git_result EQUAL 0)
	# Apply patch only if not modified.
	message(STATUS "Applying patches.")
	file(GLOB cpps ${src}/*.cpp)
	foreach(cpp IN LISTS cpps)
		file(READ ${cpp} cpp_content)
 		string(REPLACE "assert(" "injector_assert(" cpp_content "${cpp_content}")
 		string(REPLACE "std::cout" "injector::cout" cpp_content "${cpp_content}")
 		string(REPLACE "std::endl" "'\\n'" cpp_content "${cpp_content}")
 		file(WRITE ${cpp} "${cpp_content}")
 	endforeach()
else()
	message(STATUS "dspinellis/tokenizer clone already modified, not applying patches.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(
		-Wno-shorten-64-to-32
		-Wno-literal-range
	)
endif()

# tokenizer executable, based on the original makefile of tokenizer.

file(GLOB KEYWORD_FILES_FULL_PATH CONFIGURE_DEPENDS "${src}/*-keyword.txt")
file(GLOB KEYWORD_FILES RELATIVE ${src} CONFIGURE_DEPENDS "${src}/*-keyword.txt")
set(TOKENIZER_FILES "TokenizerBase.cpp")
foreach(kf IN LISTS KEYWORD_FILES)
	string(REPLACE -keyword.txt Tokenizer.cpp tf "${kf}")
	list(APPEND TOKENIZER_FILES "${tf}")
endforeach()
set(TOKENIZER_FILES_FULL_PATH ${TOKENIZER_FILES})
list(TRANSFORM TOKENIZER_FILES_FULL_PATH PREPEND "${src}/")
set(GENERATED_HEADERS
	${src}/Keyword.h
	${src}/Token.h
)
set(OBJS
	${src}/SymbolTable.cpp
	${src}/NestedClassState.cpp
)

execute_perl(mktoken.pl ${TOKENIZER_FILES} WORKING_DIRECTORY ${src})
execute_perl(mkkeyword.pl ${KEYWORD_FILES} WORKING_DIRECTORY ${src})

file(GLOB headers CONFIGURE_DEPENDS ${src}/*.h)
list(FILTER headers EXCLUDE REGEX "Test.h$")
set(all_sources 
	${OBJS}
	${src}/tokenizer.cpp
	${headers}
	${TOKENIZER_FILES_FULL_PATH}
	${KEYWORD_FILES_FULL_PATH}
	${GENERATED_HEADERS_FULL_SOURCE}
)

add_executable(tokenizer ${all_sources})
source_group(TREE ${src}/.. FILES ${all_sources})
target_precompile_headers(tokenizer PRIVATE libtokenizer/injector.h)

# libtokenizer

set(libtokenizer_sources
	libtokenizer/libtokenizer.h
	libtokenizer/libtokenizer.cpp
	libtokenizer/injector.h
	libtokenizer/injector.cpp
)
add_library(libtokenizer ${all_sources} ${libtokenizer_sources})
target_compile_definitions(libtokenizer PRIVATE LIBTOKENIZER)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${libtokenizer_sources})
target_precompile_headers(libtokenizer PRIVATE libtokenizer/injector.h)
target_include_directories(libtokenizer PUBLIC libtokenizer)

add_executable(libtokenizertest libtokenizer/libtokenizertest.cpp)
target_link_libraries(libtokenizertest PRIVATE libtokenizer)
