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
    int k = 10, n = 4, maxSize = 1024 * 4; // n：数据条带数量 k：校验条带数量
    size_t len = 1024 * 1024 * 128;                  // len：条带长度 2147483647 357913941
    size_t size = 1024 * 99 - 1;
    int thread_num = 1;
    int seed = 100;
    srand(seed);

    cout << "------------------------ Initializing data ------------------------" << endl;
    
    u8 **in, **out, *data;
    data = (u8 *)calloc(size, sizeof(u8));
    // for (int i = 0; i < size; ++i) data[i] = rand();
    scheduler sch(k, n, size);

    in = sch.strip_first_continue(data);

    // u8 *tmp = (u8 *)calloc(size, sizeof(u8));
    // in = (u8 **)calloc(k, sizeof(u8*));
    // memcpy((void *)tmp, (void *)data, size);
    // for (int i = 0; i < k; i++)
    // {
    //     in[i] = &tmp[i * len];
    // }

    printf("finish scheduler\n");
    out = (u8 **)calloc(n, sizeof(u8 *));
    for (int i = 0; i < n; ++i) out[i] = (u8 *)calloc(sch.get_strip_len(), sizeof(u8));

    // modify the len and size
    len = sch.get_strip_len();
    size = sch.get_append_total_size();
    // for (int i = 0; i < k; ++i)
    // {
    //     printf("%p ", &in[i][0]);
    // }
    // for (int i = 0; i < n; ++i)
    // {
    //     printf("%p ", &out[i][0]);
    // }
    // print_ptr(in, k, len);
    cout << "------------------------ 开始计算EC ------------------------" << endl;


    // EC校验的计算, maxSize = len / 4
    IsaEC ec(k, n, maxSize, thread_num);
    // ec.encode_ptr(in, out, size);
    // EC校验的计算
    start = chrono::high_resolution_clock::now();
    ec.encode_ptr(in, out, size);
    _end = chrono::high_resolution_clock::now();
    chrono::duration<double> _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    // print_ptr(out, n, len);

    start = chrono::high_resolution_clock::now();
    ec.encode_ptr(in, out, size);
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    start = chrono::high_resolution_clock::now();
    ec.encode_ptr(in, out, size);
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    u8 **matrix = (u8 **)calloc(k + n, sizeof(u8 *));
    for (int i = 0; i < k + n; ++i)
    {
        matrix[i] = (u8 *)calloc(len, sizeof(u8));
        if(i < k)
        {
            memcpy(matrix[i], in[i], len);
        }
        else
        {
            memcpy(matrix[i], out[i - k], len);
        }
    }
    // print_ptr(matrix, k + n, len);

    int err_num = n;
    unsigned char err_list[err_num];
    for(int i = 0; i < err_num; ++i)
    {
        err_list[i] = rand() % (k);
    }
    for (auto i : err_list)
    {
        for (size_t j = 0; j < len; j++)
        {
            matrix[(int)i][j] = 255;
        }
    }
    // print_ptr(matrix, k + n, len);
    cout << "------------------------ decode ------------------------" << endl;

    start = chrono::high_resolution_clock::now();
    ec.decode_ptr(matrix, err_num, err_list, size);
    _end = chrono::high_resolution_clock::now();


    _duration = _end - start;
    printf("decode time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (n + k) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    for (auto i: err_list)
    {
        if(memcmp(matrix[int(i)], in[(int)i], len))
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
