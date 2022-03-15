# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.0)


# 项目信息
project(RS_edr)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(lib)

aux_source_directory(./src RS_edr_SRCS)

ADD_LIBRARY(RS_edr ${RS_edr_SRCS})
add_dependencies(RS_edr build_isal)
link_directories( isa-l/bin)
include_directories( lib/isa-l/include)
TARGET_LINK_LIBRARIES(RS_edr isal)

target_include_directories(RS_edr PUBLIC src)

AUX_SOURCE_DIRECTORY(./test test_SRCS)
foreach (FILE_PATH ${test_SRCS})
    # MESSAGE(${FILE_PATH})
    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
    # MESSAGE(${FILE_NAME})
    add_executable(${FILE_NAME} ${FILE_PATH})
    TARGET_LINK_LIBRARIES(${FILE_NAME} RS_edr)

    add_custom_target(run_${FILE_NAME}
            DEPENDS ${FILE_NAME}
            COMMAND ${FILE_NAME}
            COMMENT "[ RUN] " ${FILE_NAME})
endforeach ()


include(CTest)
set(testList testEC)
foreach(testTarget ${testList})
    add_test(
    NAME TEST_${testTarget}
    COMMAND ./${testTarget}
    )
endforeach()
