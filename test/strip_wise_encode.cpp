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

void func()
{
    omp_set_num_threads(4);
    # pragma omp parallel for num_threads(4)
    for (int i = 0; i < 4; ++i)
    {
        int id = omp_get_thread_num();
        printf("id: %d\n", id);
    }
}

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
    size_t len = (size_t)1024 * 1024 * 1024 * 1; // total length for each strip
    int thread_num = 8;
    u8 **in, **out; // in：data strip out: parity strip
    u8 *tmp_in, *tmp_out, *tmp;
    int seed = 2;
    srand(seed);
    cout << "------------------------ INITIALIZE DATA ------------------------" << endl;
    cout<<"Encode strip: "<<k<<" Check sum strip: "<<n<<endl;
    cout<<"Total data: "<<(k + n) * len / GB<<"GB"<<endl;
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
        memset(in[i], rand(), len);
        // for (size_t j = 0; j < len; j++)
        //     in[i][j] = rand() % 255;
    }
    for (int i = 0; i < n; ++i)
        memset(out[i], 0, len);
        // for (size_t j = 0; j < len; ++j) out[i][j] = 0;


    cout << "------------------------ ENCODE ------------------------" << endl;
    IsaEC ec(k, n, maxSize, thread_num);

    start = chrono::high_resolution_clock::now();
    ec.encode_ptr(in, out, len);
    _end = chrono::high_resolution_clock::now();
    chrono::duration<double> _duration = _end - start;
    printf("Encode time: %f s, speed %lf GB/s \n", _duration.count(), (n + k) * len / GB / _duration.count());


    // print_ptr(in, k, len);
    // print_ptr(out, n, len);
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

    // print_ptr(matrix, k + n, len);

    cout << "------------------------ DECODE ------------------------" << endl;
    start = chrono::high_resolution_clock::now();
    ec.decode_ptr(matrix, err_num, err_list, len);
    _end = chrono::high_resolution_clock::now();

    _duration = _end - start;
    printf("decode time: %fs \n", _duration.count());
    printf("total data: %ld GB, speed %lf GB/s \n", (n + k) * len / GB, (n + k) * len / GB / _duration.count());

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