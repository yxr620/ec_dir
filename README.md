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
