#include "IsaEC.hpp"
#include <iostream>
#include <chrono>
#include <bitset>
#include<stdio.h>
#include<stdlib.h>
#include <thread>

using namespace std;

chrono::time_point<std::chrono::high_resolution_clock> start, _end;

void print_ma(vvc_u8 matrix)
{
    int line = matrix.size();
    int col = matrix[0].size();
    cout<<"Binary"<<endl;
    for(int i = 0; i < line; ++i)
    {
        for(int j = 0; j < col; ++j)
        {
            cout<<bitset<sizeof(matrix[i][j])*8>(matrix[i][j])<<' ';
        }
        cout<<endl;
    }
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

void print_ptr(u8 **matrix, int line, int col)
{
    cout<<"Binary"<<endl;
    for(int i = 0; i < line; ++i)
    {
        for(int j = 0; j < col; ++j)
        {
            cout<<bitset<sizeof(matrix[i][j])*8>(matrix[i][j])<<' ';
        }
        cout<<endl;
    }
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

int main(int argc,char *argv[])
{
    int k = atoi(argv[1]), n = 4; // n：数据条带数量 k：校验条带数量
    size_t len = 1024 * 1024 * 32; // len：条带长度
    size_t maxSize = atoi(argv[2]); // chunk size
    size_t size = k * maxSize;
    int thread_num = 1;
    u8 ***in, ***incopy; // in：测试输入 out：ec输出
    u8 *tmp_in, *tmp_out, *tmp;
    int seed = 100;
    srand(seed);
    cout << "---------------k: "<<k<<" maxSize: "<<maxSize<<"---------------"<< endl;
    cout << "------------------------ 随机初始化原数据中 ------------------------" << endl;

/*
The memory follows the following pattern

|<-maxSize->|<-maxSize->| ...   |<-maxSize->| ...
|in chunk   | in chunk  | ...   | out chunk | ...

*/
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

    cout << "------------------------ 开始计算EC ------------------------" << endl;

    IsaEC ec(k, n, maxSize, thread_num);
    // EC校验的计算
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        ec.encode_ptr(in[i], &in[i][k], size);
    }
    _end = chrono::high_resolution_clock::now();
    chrono::duration<double> _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        ec.encode_ptr(in[i], &in[i][k], size);
    }
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        ec.encode_ptr(in[i], &in[i][k], size);
    }
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        ec.encode_ptr(in[i], &in[i][k], size);
    }
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        ec.encode_ptr(in[i], &in[i][k], size);
    }
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    for (int i = 0; i < len / maxSize; ++i)
    {
        for(int j = 0; j < k + n; ++j)
            memcpy(incopy[i][j], in[i][j], maxSize);
    }

    int err_num = n;
    unsigned char err_list[err_num];
    for(int i = 0; i < err_num; ++i)
    {
        err_list[i] = rand() % (k);
    }
    for (int i = 0; i < len / maxSize; ++i)
    {
        for (auto j : err_list)
        {
            memset(in[i][j], 0, maxSize);
        }
    }

    // print_ptr(in[0], k + n, maxSize);
    // print_ptr(incopy[0], k + n, maxSize);
    cout << " ------------------------ decode ------------------------ " << endl;

    ec.cache_g_tbls(err_num, err_list);

    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        ec.cache_decode_ptr(in[i], err_num, err_list, size);
    }
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("decode time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (n + k) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    // print_ptr(in[0], k + n, maxSize);
    for (auto i: err_list)
    {
        if(incopy[0][i][0] != in[0][i][0])
        {
            cout<<"err: NOT EQUAL "<<(int)i<<endl;
            return -1;
        }
        else
        {
            cout<<"check stripe: "<<(int)i<<" EQUAL"<<endl;
        }
    }

    return 0;
}
