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
    int k = 2, n = 1, maxSize = 8; // n：数据条带数量 k：校验条带数量
    size_t len = 8;                  // len：条带长度 2147483647 357913941
    size_t size = k * len;
    int thread_num = 1;
    u8 **in, **out; // in：测试输入 out：ec输出
    u8 *tmp_in, *tmp_out;
    int seed = 100;
    srand(seed);
    cout << "------------------------ 随机初始化原数据中 ------------------------" << endl;
    in = (u8 **)calloc(k, sizeof(u8*));
    out = (u8 **)calloc(n, sizeof(u8*));

    // posix_memalign((void **)&tmp_in, 4096, k * len * sizeof(u8));
    // posix_memalign((void **)&tmp_out, 4096, n * len * sizeof(u8));
    // tmp_in = (u8 *)calloc(k * len * 2, sizeof(u8));
    // tmp_out = (u8 *)calloc(n * len * 2, sizeof(u8));
    vvc_u8 vec_in, vec_out;

    for (int i = 0; i < k; i++)
    {
        vec_in.push_back(vector<unsigned char>(len));
        in[i] = vec_in[i].data();
    }
    for (int i = 0; i < n; i++)
    {
        vec_out.push_back(vector<unsigned char>(len));
        out[i] = vec_out[i].data();
    }
    // for (int i = 0; i < k; i++)
    // {
    //     in[i] = &tmp_in[i * len];
    //     // for (size_t j = 0; j < len; j++)
    //     // {
    //     //     in[i][j] = rand() % 255;
    //     // }
    // }
    // for (int i = 0; i < n; i++)
    // {
    //     out[i] = &tmp_out[i * len];
    //     // for (size_t j = 0; j < len; j++)
    //     // {
    //     //     out[i][j] = 255;
    //     // }
    // }
    
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
    
    // print_ma(out);
    chrono::duration<double> _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());
    // print_ptr(out, n, len);

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
        if(matrix[(int)i][0] != in[(int)i][0])
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
