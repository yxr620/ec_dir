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

int main()
{
    int k = 2, n = 1, maxSize = 1024; // n：数据条带数量 k：校验条带数量
    size_t len = 1024 * 1024 * 1024;                  // len：条带长度 2147483647 357913941
    size_t size = k * len;
    int thread_num = 1;
    vvc_u8 in, out; // in：测试输入 out：ec输出
    int seed = 100;
    srand(seed);
    cout << "------------------------ 随机初始化原数据中 ------------------------" << endl;
    for (int i = 0; i < k; i++)
    {
        in.push_back(vector<unsigned char>());
        in[i].resize(len);
        for (size_t j = 0; j < len; j++)
        {
            in[i][j] = rand() % 255;
        }
    }
    for (int i = 0; i < n; i++)
    {
        out.push_back(vector<unsigned char>());
        out[i].resize(len);
        for (size_t j = 0; j < len; j++)
        {
            out[i][j] = 255;
        }
    }
    
    cout << "------------------------ 开始计算EC ------------------------" << endl;


    // EC校验的计算, maxSize = len / 4
    IsaEC ec(k, n, maxSize, thread_num);
    // EC校验的计算
    start = chrono::high_resolution_clock::now();
    ec.encode(in, out, size);
    _end = chrono::high_resolution_clock::now();
    
    // print_ma(out);
    chrono::duration<double> _duration = _end - start;
    printf("time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (k + n) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    vvc_u8 matrix;
    matrix.insert(matrix.end(), in.begin(), in.end());
    matrix.insert(matrix.end(), out.begin(), out.end());

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

    cout << "------------------------ decode ------------------------" << endl;

    start = chrono::high_resolution_clock::now();
    ec.decode(matrix, err_num, err_list, size);
    _end = chrono::high_resolution_clock::now();


    _duration = _end - start;
    printf("decode time: %fs \n", _duration.count());
    printf("total data: %ld MB, speed %lf MB/s \n", (n + k) * len / 1024 / 1024, (n + k) * len / 1024 / 1024 / _duration.count());

    for (auto i: err_list)
    {
        if(matrix[(int)i] != in[(int)i])
        {
            cout<<"err: not equal "<<(int)i<<endl;
            return -1;
        }
        else
        {
            cout<<"check stripe: "<<(int)i<<" EQUAL"<<endl;
        }
    }

    return 0;
}
