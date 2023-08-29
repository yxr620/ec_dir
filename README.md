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
