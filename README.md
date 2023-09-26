# EC编译码器
本项目为对Intel isa-l 库的封装

## 使用方法

项目设置：首先在新Cmake项目中clone本项目，然后将本项目作为子目录添加，最后连接到可执行文件
```Cmake
add_subdirectory(EC_edr)

add_executable([your_target] [your_source.cpp])
TARGET_LINK_LIBRARIES([your_target] EC_edr)
```

代码中使用：包含头文件并使用参数实例化编译码器对象

```C
#include "IsaEC.hpp"

// n = 数据条带数， k = 校验条带数， maxSize = 最大条带长度
IsaEC ec(n, k, maxSize);

ec.encode(in, out, size);

ec.decode(matrix, err_num, err_list, size);
```

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
