#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>  
#include <asm/unistd.h>
#include "IsaEC.hpp"
#include <iostream>
#include <chrono>
#include <bitset>

using namespace std;

chrono::time_point<std::chrono::high_resolution_clock> start, _end;



int main()
{
    int k = 10, n = 4; // n：数据条带数量 k：校验条带数量
    size_t maxSize = 1024 * 4; // 4k chunk size
    size_t len = 1024 * 1024 * 32; 
    size_t size = k * len;
    int thread_num = 1;
    u8 **in, **out; // in：测试输入 out：ec输出
    u8 *tmp_in, *tmp_out, *tmp;
    int seed = time(NULL);
    srand(seed);
    cout << "------------------------ 随机初始化原数据中 ------------------------" << endl;
    in = (u8 **)calloc(k, sizeof(u8*));
    out = (u8 **)calloc(n, sizeof(u8*));

    // seperate memory malloc
    for (int i = 0; i < k; i++)
    {
        posix_memalign((void **)&tmp, 64, len * sizeof(u8));
        in[i] = tmp;
    }
    for (int i = 0; i < n; i++)
    {
        posix_memalign((void **)&tmp, 64, len * sizeof(u8));
        out[i] = tmp;
    }

    // continus memory malloc
    // posix_memalign((void **)&tmp_in, 64, k * len * sizeof(u8));
    // posix_memalign((void **)&tmp_out, 64, n * len * sizeof(u8));
    // for (int i = 0; i < k; i++)
    // {
    //     in[i] = tmp_in + i * len;
    // }
    // for (int i = 0; i < n; i++)
    // {
    //     out[i] = tmp_out + i * len;
    // }


    for (int i = 0; i < k; i++)
    {
        // memset(in[i], rand(), len);
        for (size_t j = 0; j < len; j++)
            in[i][j] = rand() % 255;
    }
    for (int i = 0; i < n; ++i)
        // memset(out[i], 0, len);
        for (size_t j = 0; j < len; ++j) out[i][j] = 0;

    IsaEC ec(k, n, maxSize, thread_num);

    for (int i = 0; i < 5; ++i)
    {
        start = chrono::high_resolution_clock::now();
        ec.encode_ptr(in, out, size);
        _end = chrono::high_resolution_clock::now();
        chrono::duration<double> _duration = _end - start;
        printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    }

    return 0;
}