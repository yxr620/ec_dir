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


static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    return ret;
}

int main(int argc,char *argv[])
{
    int k = atoi(argv[1]), n = 4; // n：数据条带数量 k：校验条带数量
    size_t len = 1024*1024*32; // len：条带长度
    size_t maxSize = atoi(argv[2]); // chunk size
    size_t size = k * maxSize;
    int thread_num = 1;
    u8 ***in, ***incopy; // in：测试输入 out：ec输出
    u8 *tmp_in, *tmp_out, *tmp;
    int seed = 100;
    srand(seed);
    cout << "------------------------ k:"<<k<<"maxSixe: "<<maxSize<<"------------------------"<< endl;
    cout << "------------------------ 随机初始化原数据中 ------------------------" << endl;

/*
The memory follows the following pattern

|<-maxSize->|<-maxSize->| ...   |<-maxSize->| ...
|in chunk   | in chunk  | ...   | out chunk | ...

*/
    // in = (u8 ***)calloc(len / maxSize, sizeof(u8 **));
    // incopy = (u8 ***)calloc(len / maxSize, sizeof(u8 **));

    // for (int i = 0; i < len / maxSize; i++)
    // {
    //     in[i] = (u8 **)calloc(k + n, sizeof(u8 *));
    //     incopy[i] = (u8 **)calloc(k + n, sizeof(u8 *));

    //     for (int j = 0; j < k + n; ++j)
    //     {
    //         posix_memalign((void **)&tmp, 64, maxSize);
    //         in[i][j] = tmp;
    //         posix_memalign((void **)&tmp, 64, maxSize);
    //         incopy[i][j] = tmp;
    //     }
    // }

    tmp_in = (u8 *)calloc(len * (k + n), sizeof(u8));
    in = (u8 ***)calloc(len / maxSize, sizeof(u8 **));
    incopy = (u8 ***)calloc(len / maxSize, sizeof(u8 **));

    for (int i = 0; i < len / maxSize; i++)
    {
        in[i] = (u8 **)calloc(k + n, sizeof(u8 *));
        incopy[i] = (u8 **)calloc(k + n, sizeof(u8 *));

        for (int j = 0; j < k + n; ++j)
        {
            posix_memalign((void **)&tmp, 64, maxSize);
            in[i][j] = tmp;
            posix_memalign((void **)&tmp, 64, maxSize);
            incopy[i][j] = tmp;
        }
    }


    for (int i = 0; i < len / maxSize; ++i)
    {
        for(int j = 0; j < k + n; ++j)
            memset(in[i][j], rand(), maxSize);
            // for(int x = 0; x < maxSize; ++x)
            //     in[i][j][x] = rand();
    }

    // exit(0);
    cout << "------------------------ 开始计算EC ------------------------" << endl;

    IsaEC ec(k, n, maxSize, thread_num);




    long long count, misses, cycles, instructions,instruction_scache;
    int fd;
    struct perf_event_attr pe;
    pid_t pid;

    // 初始化 perf_event_attr 结构体
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HW_CACHE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CACHE_MISSES; // PERF_COUNT_HW_STALLED_CYCLES_FRONTEND
    pe.config1 = (0x03 << 16) | (0x00 << 8) | (0x01 << 0); // L3 cache, load operation, cache miss
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    // 打开性能计数器
    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        fprintf(stderr, "Error opening leader %llx\n", pe.config);
        exit(EXIT_FAILURE);
    }
    // 启用性能计数器
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    for (int i = 0; i < 5; ++i)
    {
        start = chrono::high_resolution_clock::now();
        for (int i = 0; i < len / maxSize; ++i)
        {
            ec.encode_ptr(in[i], &in[i][k], size);
        }
        _end = chrono::high_resolution_clock::now();
        chrono::duration<double> _duration = _end - start;
        printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    }
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &misses, sizeof(long long));
    printf("L3 cache misses: %lld\n", misses);
    close(fd);



    // 开始统计cpu-cycles
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        fprintf(stderr, "Error opening leader %llx\n", pe.config);
        exit(EXIT_FAILURE);
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    for (int i = 0; i < 5; ++i)
    {
        start = chrono::high_resolution_clock::now();
         for (int i = 0; i < len / maxSize; ++i)
        {
            ec.encode_ptr(in[i], &in[i][k], size);
        }
        _end = chrono::high_resolution_clock::now();
        chrono::duration<double> _duration = _end - start;
        printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    }
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &cycles, sizeof(long long));
    printf("CPU cycles: %lld\n", cycles);
    close(fd);


    // 开始统计instructions
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        fprintf(stderr, "Error opening instructions counter\n");
        exit(EXIT_FAILURE);
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    for (int i = 0; i < 5; ++i)
    {
        start = chrono::high_resolution_clock::now();
         for (int i = 0; i < len / maxSize; ++i)
        {
            ec.encode_ptr(in[i], &in[i][k], size);
        }
        _end = chrono::high_resolution_clock::now();
        chrono::duration<double> _duration = _end - start;
        printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    }

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &instructions, sizeof(long long));
    printf("instructions: %lld\n", instructions);
    close(fd);

    // 开始统计CYCLES_FRONTEND
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        fprintf(stderr, "Error opening CYCLES_FRONTEND counter\n");
        exit(EXIT_FAILURE);
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    for (int i = 0; i < 5; ++i)
    {
        start = chrono::high_resolution_clock::now();
         for (int i = 0; i < len / maxSize; ++i)
        {
            ec.encode_ptr(in[i], &in[i][k], size);
        }
        _end = chrono::high_resolution_clock::now();
        chrono::duration<double> _duration = _end - start;
        printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    }

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &instructions, sizeof(long long));
    printf("CYCLES_FRONTEND: %lld\n", instructions);
    close(fd);


    // 开始统计CYCLES_BACKEND
    // struct perf_event_attr pe;
    // long long count;
    // int fd;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HW_CACHE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = (PERF_COUNT_HW_CACHE_L1I) | 
                (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
       fprintf(stderr, "Error instruction cache misses %llx\n", pe.config);
       exit(EXIT_FAILURE);
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    for (int i = 0; i < 5; ++i)
    {
        start = chrono::high_resolution_clock::now();
         for (int i = 0; i < len / maxSize; ++i)
        {
            ec.encode_ptr(in[i], &in[i][k], size);
        }
        _end = chrono::high_resolution_clock::now();
        chrono::duration<double> _duration = _end - start;
        printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    }

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &instruction_scache, sizeof(long long));

    printf("instruction cache misses: %lld\n", instruction_scache);
    close(fd);

    return 0;
}