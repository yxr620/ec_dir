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
    g_tbls_decode = new unsigned char[n * k * 32];
    // 选择柯西矩阵作为编码矩阵
    gf_gen_cauchy1_matrix(encode_matrix, m, k);
    // print_encode(encode_matrix, k, m);
    // 从编码矩阵初始化 辅助编码表格
    // unsigned char c = 1;
    // unsigned char *tbl = (unsigned char *)calloc(32, sizeof(unsigned char));
    // gf_vect_mul_init(c, tbl);
    // for (int i = 0; i < 32; ++i) printf("%d ", tbl[i]);
    // exit(1);
    ec_init_tables(k, m - k, &encode_matrix[k * k], g_tbls);
}

IsaEC::~IsaEC()
{
    exit(0);
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
    return true;
}

bool IsaEC::encode_ptr(u8 **in, u8 **out, size_t size)
{
    size_t index = 0;
    u8 *frag_ptrs[m];
    int len = maxSize;

    int total_len = size / k;

    #pragma omp parallel for num_threads(thread_num)
    for(size_t index = 0; index < total_len; index += maxSize)
    {
        // printf("index %ld\n", index);
        for (int i = 0; i < m; i++)
        {
            if (i < k)
                frag_ptrs[i] = &in[i][index];
            else
                frag_ptrs[i] = &out[i - k][index];
        }
        ec_encode_data(len, k, n, g_tbls, frag_ptrs, &frag_ptrs[k]);
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

bool IsaEC::decode_ptr(u8 **matrix, int err_num, u8 *err_list, size_t size)
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

bool IsaEC::cache_g_tbls(int err_num, u8 *err_list)
{
    gf_gen_decode_matrix_simple(encode_matrix, decode_matrix,
                                invert_matrix, temp_matrix, decode_index,
                                err_list, err_num, k, m);
    ec_init_tables(k, err_num, decode_matrix, g_tbls_decode);
    return true;
}

bool IsaEC::cache_decode_ptr(u8 **matrix, int err_num, u8 *err_list, size_t size)
{
    size_t index = 0;
    int len = maxSize;


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
        ec_encode_data(len, k, err_num, g_tbls_decode, recover_srcs, recover_outp);
    }

    return true;
}

/*
TODO
*/
scheduler::scheduler(int k_, int n_, uint64_t total_size_, uint64_t chunk_size_ /*=-1*/):
k{k_}, n{n_}, total_size{total_size_}, chunk_size{chunk_size_}
{
    if (chunk_size == -1) chunk_size = 1024;
    int append_chunk = (chunk_size - (total_size % chunk_size)) % chunk_size;
    int chunk_num = (total_size + append_chunk) / chunk_size;
    int append_num = (k - (chunk_num % k)) % k;
    append_len = append_num * chunk_size + append_chunk;

    append_total_size = total_size + append_len;
    strip_len = append_total_size / k;

    printf("append_total_size %lu\nstrip_len %lu\n", append_total_size, strip_len);
    printf("append_len %lu\n", append_len);
}


/*
TODO

*/
void scheduler::initialize_para(int k_, int n_, uint64_t total_size_, uint64_t chunk_size_ /*=-1*/)
{
    if (chunk_size == -1) chunk_size = 1024;
    int append_chunk = (chunk_size - (total_size % chunk_size)) % chunk_size;
    int chunk_num = (total_size + append_chunk) / chunk_size;
    int append_num = (k - (chunk_num % k)) % k;
    append_len = append_num * chunk_size + append_chunk;

    append_total_size = total_size + append_len;
    strip_len = append_total_size / k;

    printf("append_total_size %lu\nstrip_len %lu\n", append_total_size, strip_len);
    printf("append_len %lu\n", append_len);
}

/*
return: ptr of u8 ***
|<-maxSize->|<-maxSize->| ...   |<-maxSize->| ...
|in chunk   | in chunk  | ...   | out chunk | ...
*/
u8 **scheduler::strip_first_continue(u8 *data)
{
    u8 *tmp = (u8 *)calloc(append_total_size, sizeof(u8));
    strip_first_ptr = (u8 **)calloc(k, sizeof(u8*));
    memcpy((void *)tmp, (void *)data, total_size);
    for (int i = 0; i < k; i++)
    {
        strip_first_ptr[i] = &tmp[i * strip_len];
    }
    return strip_first_ptr;
}

u8 **scheduler::strip_first_order(u8 *data)
{
    u8 *tmp = (u8 *)calloc(append_total_size, sizeof(u8));
    strip_first_ptr = (u8 **)calloc(k, sizeof(u8*));

    memcpy((void *)tmp, (void *)data, total_size);

    for (int i = 0; i < k; i++)
    {
        strip_first_ptr[i] = (u8 *)calloc(strip_len, sizeof(u8));
        if(i == k - 1)
        {
            memcpy(strip_first_ptr[i], &data[i * strip_len], strip_len - append_len);
        }
        else memcpy(strip_first_ptr[i], &data[i * strip_len], strip_len);
    }
    return strip_first_ptr;
}

u8 ***scheduler::stripe_frist_continue(u8 *data)
{
    
}

u8 ***scheduler::stripe_first_outinside(u8 *data)
{
    uint64_t stripe_size = (k) * chunk_size;
    stripe_first_ptr = (u8 ***)calloc(append_total_size / stripe_size, sizeof(u8 **));
    int data_chunk_num = total_size / chunk_size; // lower boundary integer
    for (int i = 0; i < append_total_size / stripe_size; ++i)
    {
        stripe_first_ptr[i] = (u8 **)calloc(k + n, sizeof(u8 *));

        for (int j = 0; j < k + n; ++j)
        {
            stripe_first_ptr[i][j] = (u8 *)calloc(chunk_size, sizeof(u8));
            if(j >= k || i * k + j > data_chunk_num) continue;
            else if(i * k + j == data_chunk_num)
            {
                memcpy(stripe_first_ptr[i][j], &data[(i * k + j) * chunk_size], total_size % chunk_size);
                // printf("mod %d\n", total_size % chunk_size);
            }
            else
            {
                memcpy(stripe_first_ptr[i][j], &data[(i * k + j) * chunk_size], chunk_size);
            }
        }
    }
    return stripe_first_ptr;
}

/*
TODO
*/
u8 ***scheduler::stripe_first_dynamic(u8 *data)
{

}