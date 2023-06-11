#include "IsaEC.hpp"
#include <iostream>
#include <chrono>
#include <bitset>

using namespace std;

chrono::time_point<std::chrono::high_resolution_clock> start, _end;


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

int main()
{
    int k = 10, n = 4, maxSize = 4096; // n：数据条带数量 k：校验条带数量
    size_t len = 1024 * 1024;                  // len：条带长度 2147483647 357913941
    size_t size = 1024 * 1024 * k;
    int thread_num = 1;
    int seed = 100;
    srand(seed);

    cout << "------------------------ Initializing data ------------------------" << endl;
    
    u8 ***in, ***incopy; // in：测试输入 out：ec输出
    u8 *data;
    data = (u8 *)calloc(size, sizeof(u8));
    for (int i = 0; i < size; ++i) data[i] = rand();
    scheduler sch(k, n, size, maxSize);
    in = sch.stripe_first_outinside(data);

    printf("finish scheduler\n");

    // modify the len and size
    len = sch.get_strip_len();
    size = sch.get_append_total_size();
    maxSize = sch.get_chunk_size();

    cout << "------------------------ EC encoding ------------------------" << endl;

    // print_ptr(in[10], 14, maxSize);
    IsaEC ec(k, n, maxSize, thread_num);
    // EC校验的计算
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        // printf("i %d\n", i);
        ec.encode_ptr(in[i], &in[i][k], maxSize);
    }
    _end = chrono::high_resolution_clock::now();
    chrono::duration<double> _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        ec.encode_ptr(in[i], &in[i][k], maxSize);
    }
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        ec.encode_ptr(in[i], &in[i][k], maxSize);
    }
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    u8 *tmp;
    incopy = (u8 ***)calloc(len / maxSize, sizeof(u8 **));
    for (int i = 0; i < len / maxSize; i++)
    {
        incopy[i] = (u8 **)calloc(k + n, sizeof(u8 *));
        for (int j = 0; j < k + n; ++j)
        {
            posix_memalign((void **)&tmp, 64, maxSize);
            incopy[i][j] = tmp;
        }
    }
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
    // print_ptr(incopy[0], k + n, maxSize);
    // print_ptr(in[0], k + n, maxSize);
    cout << " ------------------------ decode ------------------------ " << endl;

    ec.cache_g_tbls(err_num, err_list);

    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < len / maxSize; ++i)
    {
        ec.cache_decode_ptr(in[i], err_num, err_list, maxSize);
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
