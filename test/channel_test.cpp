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

void print_ptr(u8 **matrix, int line, int col)
{
    // cout<<"Binary"<<endl;
    // for(int i = 0; i < line; ++i)
    // {
    //     for(int j = 0; j < col; ++j)
    //     {
    //         cout<<bitset<sizeof(matrix[i][j])*8>(matrix[i][j])<<' ';
    //     }
    //     cout<<endl;
    // }
    cout<<"hex"<<endl;
    for(int i = 0; i < line; ++i)
    {
        for(int j = 0; j < col; ++j)
        {
            cout<<hex<<(int)matrix[i][j]<<' ';
        }
        cout<<endl;
    }
}

int main()
{
    int k = 8, n = 2; //k: data strip, n: parity strip
    size_t maxSize = 4 * 1024; // chunk size
    size_t len = (size_t)1024 * 1024 * 1024; // total length for each strip
    size_t parallel_size = 4 * MB; // parallel write to ssd size
    int thread_num = 16; // thread number for encoding and decoding
    int seed = 1;
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
        // for (size_t j = 0; j < len / 128; j++)
        //     in[i][j] = rand() % 255;
    }
    for (int i = 0; i < n; ++i)
        memset(out[i], 0, len);


    cout << "------------------------ WRITE SSD ------------------------" << endl;
    IsaEC ec(k, n, maxSize, thread_num);
    size_t encode_offset = 0;
    size_t write_offset = 0;
    size_t iter_len = parallel_size;


    // parallel encode and write to ssd
    start = chrono::high_resolution_clock::now();

    // parallel write to ssd without encode
    parallel_write_ssd(in, out, len, 0, k, n);

    _end = chrono::high_resolution_clock::now();
    chrono::duration<double> _duration = _end - start;
    printf("Time: %f, total data: %ld MB, speed %lf MB/s \n", _duration.count(), (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    cout << "------------------------ READ SSD ------------------------" << endl;
    u8 **read_matrix = (u8 **)calloc((k + n), sizeof(u8 *));
    for (int i = 0; i < (k + n); ++i)
    {
        posix_memalign((void **)&tmp, 4 * 1024, len * sizeof(u8));
        read_matrix[i] = tmp;
    }

    start = chrono::high_resolution_clock::now();
    parallel_read_ssd(read_matrix, len, 0, k, n);
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("decode time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (n + k) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    // print_ptr(in, k, 1024);
    // print_ptr(read_matrix, k + n, 1024);
    // check write and read result
    cout << "------------------------ CHECK ------------------------" << endl;
    for (int i = 0; i < (k + n); ++i)
    {
        if (i < k)
        {
            if (memcmp(in[i], read_matrix[i], len))
            {
                printf("read error %d\n", i);
                return -1;
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
                return -1;
            }
            else
            {
                printf("check stripe: %d EQUAL\n", i);
            }
        }
    }


    return 0;

}