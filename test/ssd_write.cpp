/*
This file is a test of ptr encoding and decoding.
The output and input can be printed using print_ptr function.
The encode and decode data is checked for correctness.
Check multithread correctness, openmp checked.
*/


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
#include <omp.h>

using namespace std;

chrono::time_point<std::chrono::high_resolution_clock> start, _end;




int main()
{
    int k = 8, n = 2; //k: data strip, n: parity strip
    size_t maxSize = 4 * 1024; // chunk size
    size_t len = 1024 * 1024 * 128; // total length for each strip
    size_t parallel_size = 1024 * 1024; // parallel write to ssd size
    int thread_num = 8; // thread number for encoding and decoding
    int seed = 1;
    char *ssd1 = "/dev/nvme1n1"; // pcie5
    char *ssd2 = "/dev/nvme2n1"; // pcie5
    char *ssd3 = "/dev/nvme3n1"; // pcie4


    size_t total_size = k * len;
    u8 **in, **out; // in：data strip out: parity strip
    u8 *tmp;
    srand(seed);

    cout << "------------------------ 随机初始化原数据中 ------------------------" << endl;
    in = (u8 **)calloc(k, sizeof(u8*));
    out = (u8 **)calloc(n, sizeof(u8*));

    // seperate memory malloc
    for (int i = 0; i < k; i++)
    {
        posix_memalign((void **)&tmp, 4 * 1024, len * sizeof(u8));
        in[i] = tmp;
    }
    for (int i = 0; i < n; i++)
    {
        posix_memalign((void **)&tmp, 4 * 1024, len * sizeof(u8));
        out[i] = tmp;
    }

    for (int i = 0; i < k; i++)
    {
        memset(in[i], rand(), len);
        for (size_t j = 0; j < len; j++)
            in[i][j] = rand() % 255;
    }
    for (int i = 0; i < n; ++i)
        memset(out[i], 0, len);


    cout << "------------------------ ENCODE ------------------------" << endl;
    IsaEC ec(k, n, maxSize, thread_num);
    int encode_offset = 0;

    // parallel encode and write to ssd
    

    for (int i = 0; i < 5; ++i)
    {
        start = chrono::high_resolution_clock::now();
        ec.encode_ptr(in, out, len);
        _end = chrono::high_resolution_clock::now();
        chrono::duration<double> _duration = _end - start;
        printf("Time: %f, total data: %ld MB, speed %lf MB/s \n", _duration.count(), (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    }

    // write to ssd
    start = chrono::high_resolution_clock::now();
    parallel_write_ssd(in, out, len, 0, k, n);
    _end = chrono::high_resolution_clock::now();
    chrono::duration<double> _duration = _end - start;
    printf("Time: %f, total data: %ld MB, speed %lf MB/s \n", _duration.count(), (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());


    cout << "------------------------ ADD ERROR ------------------------" << endl;
    u8 **matrix;
    matrix = (u8 **)calloc((k + n), sizeof(u8 *));
    for (int i = 0; i < (k + n); ++i)
    {
        posix_memalign((void **)&tmp, 64, len * sizeof(u8));
        matrix[i] = tmp;
        if (i < k)
            memcpy(matrix[i], in[i], len);
        else
            memcpy(matrix[i], out[i - k], len);
    }

    int err_num = n;
    unsigned char err_list[err_num];
    for(int i = 0; i < err_num; ++i)
    {
        err_list[i] = rand() % (k + n);
    }
    for (auto i : err_list)
    {
        for (size_t j = 0; j < len; j++)
        {
            matrix[(int)i][j] = 255;
        }
    }


    cout << "------------------------ DECODE ------------------------" << endl;
    start = chrono::high_resolution_clock::now();
    ec.decode_ptr(matrix, err_num, err_list, total_size);
    _end = chrono::high_resolution_clock::now();

    _duration = _end - start;
    printf("decode time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (n + k) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    // print_ptr(matrix, k + n, len);
    // check the decode result
    for (auto i: err_list)
    {
        if (i < k && memcmp(matrix[(int)i], in[(int)i], len))
        {
            printf("decode error\n");
            return -1;
        }
        else if (i >= k && memcmp(matrix[(int)i], out[(int)i - k], len))
        {
            printf("decode error\n");
            return -1;
        }
        else
        {
            printf("check stripe: %d EQUAL\n", (int)i);
        }
    }

    return 0;

}