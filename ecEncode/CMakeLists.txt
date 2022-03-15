# 声明要求的 cmake 最低版本
cmake_minimum_required(VERSION 3.10)

# 声明要求的 cmake 最低版本
project(enEncode VERSION 1.0)

# 设置编译模式
# set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/lib/isa-l/include)
link_directories(lib ${PROJECT_SOURCE_DIR}/lib/isa-l/bin)

add_custom_target(isa-l
WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/isa-l
BYPRODUCTS lib/isa-l/bin/*.o
COMMAND pwd; echo "\n";
COMMAND make -f ./Makefile.unx)

# 添加一个库
AUX_SOURCE_DIRECTORY(src DIR_PLOG_SRCS)
add_library(ecEncodeLib ${DIR_PLOG_SRCS})

# 添加一个可执行程序
# 语法：add_executable( 程序名 源代码文件 ）
add_executable(test test/test.cpp)

# 将库文件链接到可执行程序上
target_link_libraries(test ecEncodeLib isal)
add_dependencies(test isa-l)