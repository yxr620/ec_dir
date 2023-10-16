# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.0)


# 项目信息
project(erasure)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(lib)

aux_source_directory(./src EC_edr_SRCS)

ADD_LIBRARY(EC_edr ${EC_edr_SRCS})
add_dependencies(EC_edr build_isal)
link_directories( lib/isa-l/bin)
include_directories( lib/isa-l/include)
TARGET_LINK_LIBRARIES(EC_edr isal)
target_include_directories(EC_edr PUBLIC src)

find_package(OpenMP REQUIRED)
if(OPENMP_FOUND)
    target_link_libraries(EC_edr OpenMP::OpenMP_CXX)
endif()

message(${EC_edr_SRCS})

target_compile_options(EC_edr PUBLIC -O3)

AUX_SOURCE_DIRECTORY(./test test_SRCS)
foreach (FILE_PATH ${test_SRCS})
    # MESSAGE(${FILE_PATH})
    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
    # MESSAGE(${FILE_NAME})
    add_executable(${FILE_NAME} ${FILE_PATH})
    TARGET_LINK_LIBRARIES(${FILE_NAME} EC_edr)

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
