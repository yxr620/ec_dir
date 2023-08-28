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

void wrapper(IsaEC ec, u8 **in, u8 **out, size_t size)
{
    for (int i = 0; i < 5; ++i)
    {
        start = chrono::high_resolution_clock::now();
        ec.encode_ptr(in, out, size);
        _end = chrono::high_resolution_clock::now();
        chrono::duration<double> _duration = _end - start;
        printf("time: %fs \n", _duration.count());
        // printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    }
}

int main()
{
    int k = 160, n = 4; // n：数据条带数量 k：校验条带数量
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
    // for (int i = 0; i < k; i++)
    // {
    //     posix_memalign((void **)&tmp, 64, len * sizeof(u8));
    //     in[i] = tmp;
    // }
    // for (int i = 0; i < n; i++)
    // {
    //     posix_memalign((void **)&tmp, 64, len * sizeof(u8));
    //     out[i] = tmp;
    // }

    // continues memory malloc
    int Sep = 1024;
    posix_memalign((void **)&tmp_in, 64, k * (len * sizeof(u8)  + Sep)  ) ;
    posix_memalign((void **)&tmp_out, 64, n * len * sizeof(u8));
    for(int i = 0; i < k; ++i)
    {
        in[i] = &tmp_in[i * (len + Sep)];
    }
    for (int i = 0; i < n; ++i)
    {
        out[i] = &tmp_out[i * len];
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

    for (int i = 0; i < k; ++i)
    {
        printf("%p ", in[i]);
    }
    // exit(0);
    cout << "------------------------ 开始计算EC ------------------------" << endl;


    // EC校验的计算, maxSize = len / 4
    IsaEC ec(k, n, maxSize, thread_num);
    // EC校验的计算

    // wrapper(ec, in, out, size);

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
