#pragma once

#include <iostream>
#include <cstring>
#include <vector>
#include <limits.h>
#include <pthread.h>
#include "erasure_code.h"

using namespace std;

typedef unsigned char u8;
typedef vector<vector<u8> > vvc_u8;


class IsaEC
{
private:
    int n; // data 
    int k; // check
    int m; // total
    int maxSize;
    int thread_num;
    u8 *encode_matrix; // 编码矩阵（生成矩阵）
    u8 *decode_matrix; // 解码矩阵
    u8 *invert_matrix; // 逆矩阵
    u8 *temp_matrix;   // 临时矩阵
    u8 *g_tbls;        // 辅助编码表格
    u8 *decode_index;  // 解码矩阵的索引

public:
    IsaEC(int n_, int k_, int maxSize_, int thread_num_=4);
    ~IsaEC();
    bool encode(vvc_u8 &in, vvc_u8 &out, size_t size);
    bool encode_ptr(u8 **in, u8 **out, size_t size);
    bool decode(vvc_u8 &matrix, int err_num, u8 *err_list, size_t size);
    int getMinSize();

    /*
     * Generate decode matrix from encode matrix and erasure list
     * 由 编码矩阵 和 擦除列表 生成 解码矩阵
     */

    static int gf_gen_decode_matrix_simple(u8 *encode_matrix,
                                           u8 *decode_matrix,
                                           u8 *invert_matrix,
                                           u8 *temp_matrix,
                                           u8 *decode_index, u8 *frag_err_list, int nerrs, int n,
                                           int m)
    {
        int i, j, k, r;
        int nsrcerrs = 0;
        u8 s, *b = temp_matrix;
        u8 frag_in_err[m];

        memset(frag_in_err, 0, sizeof(frag_in_err));

        // Order the fragments in erasure for easier sorting
        for (i = 0; i < nerrs; i++)
        {
            if (frag_err_list[i] < n)
                nsrcerrs++;
            frag_in_err[frag_err_list[i]] = 1;
        }

        // Construct b (matrix that encoded remaining frags) by removing erased rows
        for (i = 0, r = 0; i < n; i++, r++)
        {
            while (frag_in_err[r])
                r++;
            for (j = 0; j < n; j++)
                b[n * i + j] = encode_matrix[n * r + j];
            decode_index[i] = r;
        }

        // Invert matrix to get recovery matrix
        if (gf_invert_matrix(b, invert_matrix, n) < 0)
            return -1;

        // Get decode matrix with only wanted recovery rows
        for (i = 0; i < nerrs; i++)
        {
            if (frag_err_list[i] < n) // A src err
                for (j = 0; j < n; j++)
                    decode_matrix[n * i + j] =
                        invert_matrix[n * frag_err_list[i] + j];
        }

        // For non-src (parity) erasures need to multiply encode matrix * invert
        for (k = 0; k < nerrs; k++)
        {
            if (frag_err_list[k] >= n)
            { // A parity err
                for (i = 0; i < n; i++)
                {
                    s = 0;
                    for (j = 0; j < n; j++)
                        s ^= gf_mul(invert_matrix[j * n + i],
                                    encode_matrix[n * frag_err_list[k] + j]);
                    decode_matrix[n * k + i] = s;
                }
            }
        }
        return 0;
    }
};

