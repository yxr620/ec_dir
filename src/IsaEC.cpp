#include "IsaEC.hpp"
#include <pthread.h>
#include <omp.h>
#include <bitset>
#include <iostream>

using namespace std;

void print_encode(u8 *matrix, int k, int m)
{
    printf("Matrix: \n");
    for (int i = 0; i < m; ++i)
    {
        for(int j = 0; j < k; ++j)
        {
            cout<<hex<<(int)matrix[i * k + j]<<' ';
        }
        cout<<endl;
    }
}

IsaEC::IsaEC(int k_, int n_, int maxSize_, int thread_num_/*=4*/) : n(n_), k(k_), maxSize(maxSize_), m(n_ + k_), thread_num(thread_num_)
{
    // 各种中间矩阵的空间分配
    decode_index = new unsigned char[m];
    encode_matrix = new unsigned char[m * k];
    decode_matrix = new unsigned char[m * k];
    invert_matrix = new unsigned char[m * k];
    temp_matrix = new unsigned char[m * k];
    g_tbls = new unsigned char[n * k * 32];
    // 选择柯西矩阵作为编码矩阵
    gf_gen_cauchy1_matrix(encode_matrix, m, k);
    print_encode(encode_matrix, k, m);
    // 从编码矩阵初始化 辅助编码表格
    ec_init_tables(k, m - k, &encode_matrix[k * k], g_tbls);
}

IsaEC::~IsaEC()
{
    delete[] encode_matrix;
    delete[] decode_index;
    delete[] decode_matrix;
    delete[] invert_matrix;
    delete[] temp_matrix;
    delete[] g_tbls;
}

bool IsaEC::encode(vvc_u8 &in, vvc_u8 &out, size_t size)
{
    size_t index = 0;
    u8 *frag_ptrs[m];
    int len = maxSize;


    int total_len = size / k;
    //change to for loop
    #pragma omp parallel for num_threads(thread_num)
    for(size_t index = 0; index < total_len; index += maxSize)
    {
        // int id = omp_get_thread_num();
        // printf("id: %d\n", id);
        for (int i = 0; i < m; i++)
        {
            if (i < k)
                frag_ptrs[i] = &in[i][index];
            else
                frag_ptrs[i] = &out[i - k][index];
        }
        // printf("start encode\n");
        ec_encode_data(len, k, n, g_tbls, frag_ptrs, &frag_ptrs[k]);
    }

    // while (index < size / n)
    // {
    //     if ((index + maxSize) > size / n)
    //     {
    //         len = size / n - index;
    //     }
    //     // cout<<"当前ec计算位置 index:"<<index<<" len:"<<len<<endl;
    //     for (int i = 0; i < m; i++)
    //     {
    //         if (i < n)
    //             frag_ptrs[i] = &in[i][index];
    //         else
    //             frag_ptrs[i] = &out[i - n][index];
    //     }
    //     ec_encode_data(len, n, k, g_tbls, frag_ptrs, &frag_ptrs[n]);
    //     index += maxSize;
    // }

    return true;
}

bool IsaEC::encode_ptr(u8 **in, u8 **out, size_t size)
{
    size_t index = 0;
    u8 *frag_ptrs[m];
    int len = maxSize;
    while (index < size / n)
    {
        if ((index + maxSize) > size / n)
        {
            len = size / n - index;
        }
        // cout<<"当前ec计算位置 index:"<<index<<" len:"<<len<<endl;
        for (int i = 0; i < m; i++)
        {
            if (i < n)
                frag_ptrs[i] = &in[i][index];
            else
                frag_ptrs[i] = &out[i - n][index];
        }
        ec_encode_data(len, n, k, g_tbls, frag_ptrs, &frag_ptrs[n]);
        index += maxSize;
    }

    return true;
}

bool IsaEC::decode(vvc_u8 &matrix, int err_num, u8 *err_list, size_t size)
{
    size_t index = 0;
    int len = maxSize;

    gf_gen_decode_matrix_simple(encode_matrix, decode_matrix,
                                invert_matrix, temp_matrix, decode_index,
                                err_list, err_num, k, m);
    ec_init_tables(k, err_num, decode_matrix, g_tbls);

    int total_len = size / k;
    //change to for loop
    #pragma omp parallel for num_threads(thread_num)
    for(size_t index = 0; index < total_len; index += maxSize)
    {
        u8 *recover_outp[err_num];
        u8 *recover_srcs[k]; // 片段缓冲区的指针
        // int id = omp_get_thread_num();
        // printf("id: %d\n", id);

        for (int i = 0; i < k; i++)
            recover_srcs[i] = &matrix[decode_index[i]][index];
        // 初始化辅助编码表格
        // 将recover_outp的地址直接设置为matrix对应位置
        for (int i = 0; i < err_num; i++)
            recover_outp[i] = &matrix[err_list[i]][index];

        ec_encode_data(len, k, err_num, g_tbls, recover_srcs, recover_outp);
    }

    return true;
}
