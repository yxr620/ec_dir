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
    int k = 8, n = 4; //k: data strip, n: parity strip
    size_t maxSize = 4 * 1024; // chunk size
    size_t len = (size_t)1024 * 1024 * 1024; // total length for each strip
    size_t parallel_size = 4 * MB; // parallel write to ssd size
    int thread_num = 16; // thread number for encoding and decoding
    int seed = time(NULL);
    const char *ssd1 = "/dev/nvme1n1"; // pcie5
    const char *ssd2 = "/dev/nvme2n1"; // pcie5
    const char *ssd3 = "/dev/nvme3n1"; // pcie4


    size_t total_size = k * len;
    u8 **in, **out; // in：data strip out: parity strip
    u8 *tmp;
    srand(seed);

    cout << "------------------------ INITIALIZE DATA ------------------------" << endl;
    in = (u8 **)calloc(k, sizeof(u8*));
    out = (u8 **)calloc(n, sizeof(u8*));

    // seperate memory malloc
    for (int i = 0; i < k; i++)
    {
        int dump = posix_memalign((void **)&tmp, 4 * 1024, len * sizeof(u8));
        in[i] = tmp;
    }
    for (int i = 0; i < n; i++)
    {
        int dump = posix_memalign((void **)&tmp, 4 * 1024, len * sizeof(u8));
        out[i] = tmp;
    }

    for (int i = 0; i < k; i++)
    {
        memset(in[i], rand(), len);
        // for (size_t j = 0; j < len / 128; j++)
        //     in[i][j] = rand() % 255;
    }
    for (int i = 0; i < n; ++i)
        memset(out[i], rand(), len);


    cout << "------------------------ WRITE SSD ------------------------" << endl;

    size_t strip_offset = (unsigned long)1024 * 1024 * 1024 * 10;

    cout<<"PCIE 5 WRITE"<<endl;
    start = chrono::high_resolution_clock::now();
    # pragma omp parallel for num_threads(k / 2)
    for (int i = 0; i < k / 2; ++i)
    {
        size_t in_offset = i * strip_offset;
        write_ssd(in[i], len, in_offset, ssd1);
    }
    _end = chrono::high_resolution_clock::now();
    chrono::duration<double> _duration = _end - start;
    printf("Time: %f, total data: %ld GB, speed %lf Gbps \n", _duration.count(), k / 2 * len / GB, k / 2 * len / GB * 8 / _duration.count());

    cout<<"PCIE 4 WRITE"<<endl;
    start = chrono::high_resolution_clock::now();
    # pragma omp parallel for num_threads(n)
    for (int i = k; i < k + n; ++i)
    {
        size_t in_offset = (i - k) * strip_offset;
        write_ssd(out[i - k], len, in_offset, ssd3);
    }
    _duration = chrono::high_resolution_clock::now() - start;
    printf("Time: %f, total data: %ld GB, speed %lf Gbps \n", _duration.count(), n * len / GB, n * len / GB * 8 / _duration.count());


    cout << "------------------------ READ SSD ------------------------" << endl;
    u8 **read_matrix = (u8 **)calloc((k + n), sizeof(u8 *));
    for (int i = 0; i < (k + n); ++i)
    {
        int dump = posix_memalign((void **)&tmp, 4 * 1024, len * sizeof(u8));
        read_matrix[i] = tmp;
    }

    cout<<"PCIE 5 READ"<<endl;
    start = chrono::high_resolution_clock::now();
    # pragma omp parallel for num_threads(k / 2)
    for (int i = 0; i < k / 2; ++i)
    {
        size_t in_offset = i * strip_offset;
        read_ssd(read_matrix[i], len, in_offset, ssd1);
    }
    _duration = chrono::high_resolution_clock::now() - start;
    printf("decode time: %fs \n", _duration.count());
    printf("total data: %ld GB, speed %lf Gbps \n", k / 2 * len / GB, k / 2 * len / GB * 8 / _duration.count());

    cout<<"PCIE 4 READ"<<endl;
    start = chrono::high_resolution_clock::now();
    # pragma omp parallel for num_threads(n)
    for (int i = k; i < k + n; ++i)
    {
        size_t in_offset = (i - k) * strip_offset;
        read_ssd(read_matrix[i], len, in_offset, ssd3);
    }
    _duration = chrono::high_resolution_clock::now() - start;
    printf("decode time: %fs \n", _duration.count());
    printf("total data: %ld GB, speed %lf Gbps \n", n * len / GB, n * len / GB * 8 / _duration.count());

    // print_ptr(in, k, 1024);
    // print_ptr(read_matrix, k + n, 1024);
    // check write and read result
    cout << "------------------------ CHECK ------------------------" << endl;
    for (int i = 0; i < (k + n); ++i)
    {
        if (i < k)
        {
            if (i >= k / 2) continue;
            if (memcmp(in[i], read_matrix[i], len))
            {
                printf("read error %d\n", i);
                // return -1;
            }
            else
            {
                printf("check stripe: %d EQUAL\n", i);
            }
        }
        else
        {
            if (memcmp(out[i - k], read_matrix[i], len))
            {
                printf("read error %d\n", i);
                // return -1;
            }
            else
            {
                printf("check stripe: %d EQUAL\n", i);
            }
        }
    }


    return 0;

}