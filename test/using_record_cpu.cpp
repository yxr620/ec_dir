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

    // 以下为perf record内容
    int i;
    int fd1, fd2;
    struct perf_event_attr pe1, pe2;
    pid_t pid;

    // 初始化 perf_event_attr 结构体
    memset(&pe1, 0, sizeof(struct perf_event_attr));
    pe1.type = PERF_TYPE_HARDWARE;
    pe1.size = sizeof(struct perf_event_attr);
    pe1.config = PERF_COUNT_HW_CPU_CYCLES;
    pe1.disabled = 1;
    pe1.exclude_kernel = 1;
    pe1.exclude_hv = 1;

    memset(&pe2, 0, sizeof(struct perf_event_attr));
    pe2.type = PERF_TYPE_HARDWARE;
    pe2.size = sizeof(struct perf_event_attr);
    pe2.config = PERF_COUNT_HW_CACHE_MISSES;
    pe2.disabled = 1;
    pe2.exclude_kernel = 1;
    pe2.exclude_hv = 1;

    // 打开性能计数器
    fd1 = perf_event_open(&pe1, 0, -1, -1, 0);
    if (fd1 == -1) {
        fprintf(stderr, "Error opening leader %llx\n", pe1.config);
        exit(EXIT_FAILURE);
    }

    fd2 = perf_event_open(&pe2, 0, -1, -1, 0);
    if (fd2 == -1) {
        fprintf(stderr, "Error opening leader %llx\n", pe2.config);
        exit(EXIT_FAILURE);
    }

    // 启用性能计数器
    ioctl(fd1, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd1, PERF_EVENT_IOC_ENABLE, 0);

    ioctl(fd2, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd2, PERF_EVENT_IOC_ENABLE, 0);

    for (int i = 0; i < 5; ++i)
    {
        start = chrono::high_resolution_clock::now();
        ec.encode_ptr(in, out, size);
        _end = chrono::high_resolution_clock::now();
        chrono::duration<double> _duration = _end - start;
        printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    }

    // 停止性能计数器
    ioctl(fd1, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(fd2, PERF_EVENT_IOC_DISABLE, 0);

    // 将性能计数器的信息写入perf record文件中
    char command[1024];
    sprintf(command, "perf record -e cycles -e cache-misses -o perf.data -p %d", getpid());
    system(command);

    close(fd1);
    close(fd2);
    return 0;
}