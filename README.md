# EC编译码器
本项目为对Intel isa-l 库的封装

## 编译该项目

创建build目录，并且生成编译指令：

```bash
mkdir build
cmake -B build
```

进入build目录并且编译项目：

```bash
cd build
make
```

## 测试在SSD上编码的速度

进入build目录后可以直接使用`make run`指令一键编译并且运行test目录下的文件。

使用如下指令可以测试编码写入SSD的速度，已经读出SSD数据解码速度：

```bash
sudo make run_para_ssd_read
```
由于在写入SSD时需要sudo权限，因此在指令之前需要加入sudo指令。具体通过如下参数指定写入具体某个SSD，以及编码格式的相关参数。

```C++
int k = 8, n = 2; //k: data strip, n: parity strip
size_t maxSize = 4 * 1024; // chunk size
size_t len = (size_t)1024 * 1024 * 1024; // total length for each strip
size_t parallel_size = MB; // parallel write to ssd size
int thread_num = 16; // thread number for encoding and decoding
int seed = 10;
const char *ssd1 = "/dev/nvme1n1"; // pcie5
const char *ssd2 = "/dev/nvme2n1"; // pcie5
const char *ssd3 = "/dev/nvme3n1"; // pcie4
```
如果要查询全部SSD的名称可以使用`sudo fdisk -l`

上述程序在SSD上编解码的性能时会给出实时显示表示编解码的性能：
```
------------------------ INITIALIZE DATA ------------------------
------------------------ ENCODE ------------------------
thread num: 16
Time: 1.386815, total data: 10240 MB, speed 7383.826380 MB/s 
------------------------ READ SSD ------------------------
check stripe: 0 EQUAL
check stripe: 1 EQUAL
check stripe: 2 EQUAL
check stripe: 3 EQUAL
check stripe: 4 EQUAL
check stripe: 5 EQUAL
check stripe: 6 EQUAL
check stripe: 7 EQUAL
check stripe: 8 EQUAL
check stripe: 9 EQUAL
------------------------ ADD ERROR ------------------------
------------------------ DECODE ------------------------
finish
decode time: 0.584768s 
total data: 10240 MB, speed 17511.226120 MB/s 
check stripe: 9 EQUAL
check stripe: 1 EQUAL
[100%] Built target run_para_ssd_read
```
其中ENCODE部分展示了编码过程的速度。后续READ SSD部分检查写入SSD的数据是否能被正确的读出。最后DECODE部分展示了解码部分是否正确，以及解码的速度。


## Perf测试

using_stat_all.cpp测试数据条优先的硬件事件结果，using_stat_all_stripe.cpp测试编码条带优先的硬件事件结果。使用C语言直接条用perf库测试指定硬件事件结果主要通过config实现。

1. 测试L3 cache miss的结果

```C
pe.config = (PERF_COUNT_HW_CACHE_LL) |
            (PERF_COUNT_HW_CACHE_OP_READ << 8) |
            (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
```

2. 测试L1 instruction cache miss结果

```C
pe.config = (PERF_COUNT_HW_CACHE_L1I) | 
            (PERF_COUNT_HW_CACHE_OP_READ << 8) |
            (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
```

3. 测试L1 data cache miss结果

```C
pe.config = (PERF_COUNT_HW_CACHE_L1D) |
            (PERF_COUNT_HW_CACHE_OP_READ << 8) |
            (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
```

## cmake解释

下面这段代码的唯一作用是定义变量`PROJECT_NAME`

```cmake
project(erasure)
```

之后初始化库变量`EC_edr`，即先声明`EC_edr`然后给其赋值为`EC_edr_SRCS`

```cmake
ADD_LIBRARY(EC_edr ${EC_edr_SRCS})
```

接下来给创建的库添加依赖等操作：
```cmake
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
```
