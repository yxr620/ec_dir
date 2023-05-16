#include "IsaEC.hpp"
#include <iostream>
#include <chrono>
#include <bitset>

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

int main()
{
    int k = 160, n = 4, maxSize = 1024; // n：数据条带数量 k：校验条带数量
    size_t len = 1024 * 1024 * 32;                  // len：条带长度 2147483647 357913941
    size_t size = k * len;
    int thread_num = 1;
    u8 **in, **out; // in：测试输入 out：ec输出
    u8 *tmp_in, *tmp_out, *tmp;
    int seed = 100;
    srand(seed);
    cout << "------------------------ 随机初始化原数据中 ------------------------" << endl;
    in = (u8 **)calloc(k, sizeof(u8*));
    out = (u8 **)calloc(n, sizeof(u8*));

    // posix_memalign((void **)&tmp_in, 64, k * len * sizeof(u8));
    // posix_memalign((void **)&tmp_out, 64, n * len * sizeof(u8));
    // tmp_in = (u8 *)calloc(k * len * 2, sizeof(u8));
    // tmp_out = (u8 *)calloc(n * len * 2, sizeof(u8));


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
    

    cout << "------------------------ 开始计算EC ------------------------" << endl;


    // EC校验的计算, maxSize = len / 4
    IsaEC ec(k, n, maxSize, thread_num);
    // EC校验的计算
    start = chrono::high_resolution_clock::now();
    ec.encode_ptr(in, out, size);
    _end = chrono::high_resolution_clock::now();
    
    // print_ma(out);
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

    start = chrono::high_resolution_clock::now();
    ec.encode_ptr(in, out, size);
    _end = chrono::high_resolution_clock::now();
    _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    return 0;
}
