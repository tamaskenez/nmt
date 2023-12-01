set(ABSL_GIT_REPOSITORY https://github.com/abseil/abseil-cpp)
#set(GTEST_GIT_REPOSITORY https://github.com/google/googletest.git)
set(GTEST_GIT_REPOSITORY https://github.com/tamaskenez/googletest.git)
set(GTEST_GIT_BRANCH cmake_external_absl_re2)
set(RE2_GIT_REPOSITORY https://github.com/google/re2.git)

if(NOT CMAKE_INSTALL_PREFIX)
    message(FATAL_ERROR "Missing CMAKE_INSTALL_PREFIX")
endif()
if(NOT BUILD_DIR)
    message(FATAL_ERROR "Missing CMAKE_INSTALL_PREFIX")
endif()
if(EXISTS ${BUILD_DIR})
    if(NOT IS_DIRECTORY ${BUILD_DIR})
        message(FATAL_ERROR "BUILD_DIR is not a directory: ${BUILD_DIR}")
    endif()
else()
    file(MAKE_DIRECTORY ${BUILD_DIR})
endif()

find_package(Git REQUIRED)

# Helper macros to execute git commands.
macro(cmake)
    execute_process(COMMAND ${CMAKE_COMMAND} ${ARGV}
        COMMAND_ERROR_IS_FATAL ANY)
endmacro()

foreach(project ABSL RE2 GTEST)
    set(s ${BUILD_DIR}/${project}/s)
    if(NOT IS_DIRECTORY ${s})
        if(DEFINED ${project}_GIT_BRANCH)
            set(git_branch_option --branch ${${project}_GIT_BRANCH})
        endif()
        execute_process(
            COMMAND ${GIT_EXECUTABLE}
                clone --depth 1 ${git_branch_option}
                    ${${project}_GIT_REPOSITORY} ${s}
            COMMAND_ERROR_IS_FATAL ANY
        )
    else()
        message(STATUS "Not cloning ${project}, directory exists")
    endif()
endforeach()

foreach(project ABSL RE2 GTEST) # dspinellis_tokenizer is moved into the main project.
    if(project STREQUAL dspinellis_tokenizer)
        set(s ${CMAKE_CURRENT_LIST_DIR}/dspinellis_tokenizer)
    else()
        set(s ${BUILD_DIR}/${project}/s)
    endif()
    set(b ${BUILD_DIR}/${project}/b)
    cmake(-S ${s} -B ${b}
        -D CMAKE_BUILD_TYPE=Debug
        -D CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -D CMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
        -D BUILD_SHARED_LIBS=0
        -D CMAKE_DEBUG_POSTFIX=_d
        -D CMAKE_CXX_STANDARD=23
        -D CMAKE_CXX_STANDARD_REQUIRED=1
        # ABSL
        -D BUILD_TESTING=0
        -D ABSL_PROPAGATE_CXX_STD=1
        # GTEST
        -D GTEST_HAS_ABSL=1
    )
    cmake(--build ${b} --target install --config Debug -j)
    cmake(${b} -DCMAKE_BUILD_TYPE=Release)
    cmake(--build ${b} --target install --config Release -j)
endforeach()
