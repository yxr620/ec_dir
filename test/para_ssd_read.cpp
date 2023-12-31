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

#define GB ((size_t)1024 * 1024 * 1024)
#define MB ((size_t)1024 * 1024)
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
    size_t parallel_size = MB; // parallel write to ssd size
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


    cout << "------------------------ ENCODE ------------------------" << endl;
    IsaEC ec(k, n, maxSize, thread_num);
    size_t encode_offset = 0;
    size_t write_offset = 0;
    size_t iter_len = parallel_size;


    // parallel encode and write to ssd
    start = chrono::high_resolution_clock::now();

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            // printf("thread %d\n", omp_get_thread_num());
            while(encode_offset < len) 
            {
                u8 **in_offset = (u8 **)calloc(k, sizeof(u8*));
                u8 **out_offset = (u8 **)calloc(n, sizeof(u8*));
                for (int i = 0; i < k; i++)
                {
                    in_offset[i] = in[i] + encode_offset;
                }
                for (int i = 0; i < n; i++)
                {
                    out_offset[i] = out[i] + encode_offset;
                }
                ec.encode_ptr(in_offset, out_offset, iter_len);
                encode_offset += iter_len;
            }
        }
        
        #pragma omp section
        {
            int id = omp_get_thread_num();
            while (write_offset < len)
            {
                while (write_offset < encode_offset)
                    continue;
                
                u8 **in_offset = (u8 **)calloc(k, sizeof(u8*));
                u8 **out_offset = (u8 **)calloc(n, sizeof(u8*));
                for (int i = 0; i < k; i++)
                {
                    in_offset[i] = in[i] + write_offset;
                }
                for (int i = 0; i < n; i++)
                {
                    out_offset[i] = out[i] + write_offset;
                }
                parallel_write_ssd(in_offset, out_offset, iter_len, write_offset, k, n);
                write_offset += iter_len;
            }
        }
    }
    #pragma omp taskwait

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
    for (int i = 0; i < (k + n); ++i)
    {
        if (i < k / 2)
        {
            read_ssd(read_matrix[i], len, i * 10 * GB, ssd1);
        }
        else if (i < k)
        {
            read_ssd(read_matrix[i], len, (i - k / 2) * 10 * GB, ssd2);
        }
        else
        {
            read_ssd(read_matrix[i], len, (i - k) * 10 * GB, ssd3);
        }
    }

    // print_ptr(in, k, 1024);
    // print_ptr(read_matrix, k + n, 1024);
    // check write and read result
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

    cout << "------------------------ ADD ERROR ------------------------" << endl;
    u8 **matrix;
    matrix = (u8 **)calloc((k + n), sizeof(u8 *));
    for (int i = 0; i < (k + n); ++i)
    {
        posix_memalign((void **)&tmp, 4 * 1024, len * sizeof(u8));
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
    // parallel read
    start = chrono::high_resolution_clock::now();
    parallel_read_ssd(matrix, len, 0, k, n);

    ec.decode_ptr(matrix, err_num, err_list, len);
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
            printf("decode error %d\n", (int)i);
            return -1;
        }
        else if (i >= k && memcmp(matrix[(int)i], out[(int)i - k], len))
        {
            printf("decode error %d\n", (int)i);
            return -1;
        }
        else
        {
            printf("check stripe: %d EQUAL\n", (int)i);
        }
    }

    return 0;

}