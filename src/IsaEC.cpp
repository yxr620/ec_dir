#include "IsaEC.hpp"

IsaEC::IsaEC(int n_, int k_, int maxSize_) : n(n_), k(k_), maxSize(maxSize_), m(n_ + k_)
{
    // 各种中间矩阵的空间分配
    decode_index = new unsigned char[m];
    encode_matrix = new unsigned char[m * n];
    decode_matrix = new unsigned char[m * n];
    invert_matrix = new unsigned char[m * n];
    temp_matrix = new unsigned char[m * n];
    g_tbls = new unsigned char[n * k * 32];
    // 选择柯西矩阵作为编码矩阵
    gf_gen_cauchy1_matrix(encode_matrix, m, n);
    // 从编码矩阵初始化 辅助编码表格
    ec_init_tables(n, k, &encode_matrix[n * n], g_tbls);
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
    u8 *recover_outp[err_num];
    u8 *recover_srcs[n]; // 片段缓冲区的指针
    size_t index = 0;
    int len = maxSize;
    while (index < size / n)
    {
        if ((index + maxSize) > size / n)
        {
            len = size / n - index;
        }
        // cout<<"当前ec恢复位置 index:"<<index<<" len:"<<len<<endl;
        // 生成解码矩阵
        gf_gen_decode_matrix_simple(encode_matrix, decode_matrix,
                                    invert_matrix, temp_matrix, decode_index,
                                    err_list, err_num, n, m);
        // 将恢复数组指针打包为有效片段列表
        for (int i = 0; i < n; i++)
            recover_srcs[i] = &matrix[decode_index[i]][index];
        // 初始化辅助编码表格
        ec_init_tables(n, err_num, decode_matrix, g_tbls);
        // 将recover_outp的地址直接设置为matrix对应位置
        for (int i = 0; i < err_num; i++)
            recover_outp[i] = &matrix[err_list[i]][index];
        // 开始恢复
        ec_encode_data(len, n, err_num, g_tbls, recover_srcs, recover_outp);
        index += maxSize;
    }
    return true;
}