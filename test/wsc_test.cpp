#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <isa-l.h>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring> // add this header for memset()
#include <assert.h>

using namespace std;

#define MMAX 200
#define KMAX 200
#define STRIPEMAX (128 * 1024LL)
#define NUM_THREADS (1)
#define LOOP (1)
// #define DATASIZE_ALOOP (2 * 1024 * 1024 * 1024ULL)
#define DATASIZE_ALOOP (1 * 1024 * 1024 * 1024ULL)

typedef unsigned char u8;

struct thread_data
{
    int thread_id;
    uint8_t *data;
    uint8_t **encoded_data;
    uint8_t *decoded_data;
    uint8_t *decode_matrix;
    uint8_t *invert_matrix;
    uint8_t *g_tbls;
};

int usage(void)
{
    fprintf(stderr,
            "Usage: ec_simple_example [options]\n"
            "  -h        Help\n"
            "  -k <val>  Number of source fragments\n"
            "  -p <val>  Number of parity fragments\n"
            "  -l <val>  Length of fragments\n"
            "  -e <val>  Simulate erasure on frag index val. Zero based. Can be repeated.\n"
            "  -r <seed> Pick random (k, p) with seed\n");
    exit(0);
}

static int gf_gen_decode_matrix_simple(u8 *encode_matrix,
                                       u8 *decode_matrix,
                                       u8 *invert_matrix,
                                       u8 *temp_matrix,
                                       u8 *decode_index,
                                       u8 *frag_err_list, int nerrs, int k, int m);

void encode_data(u8 ***data_ptrs, u8 *encode_matrix, u8 *g_tbls, int k, int p, int len, int start_offset, int total_stripes)
{
    // cout << total_stripes << endl;
    for (size_t loop = 0; loop < LOOP; loop++)
    {
        for (size_t i = 0; i < total_stripes; i++)
        {
            // uint64_t offset = start_offset + i;
            // Generate EC parity blocks from sources
            ec_encode_data(len, k, p, g_tbls, data_ptrs[start_offset + i], &data_ptrs[start_offset + i][k]);
        }
    }

    return;
}

int main(int argc, char *argv[])
{
    uint64_t data_size = DATASIZE_ALOOP; // 1GB
    int i, j, m, c, e, ret;
    int k = 160, p = 4, len = 1 * 1024; // Default params
    m = k + p;
    int nerrs = 0;
    int stripesize = data_size / (len * k);
    assert(stripesize < STRIPEMAX);

    // Fragment buffer pointers
    u8 ***frag_ptrs;
    // u8 *recover_srcs[KMAX];
    // u8 *recover_outp[KMAX];
    // u8 frag_err_list[STRIPEMAX][MMAX];

    // Coefficient matrices
    u8 *encode_matrix, *decode_matrix;
    u8 *invert_matrix, *temp_matrix;
    u8 *g_tbls;
    u8 decode_index[MMAX];

    // Initialize data with random values
    srand((unsigned)time(NULL));

    // Check for valid parameters
    if (m > MMAX || k > KMAX || m < 0 || p < 1 || k < 1)
    {
        printf(" Input test parameter error m=%d, k=%d, p=%d, erasures=%d\n",
               m, k, p, nerrs);
        usage();
    }
    if (nerrs > p)
    {
        printf(" Number of erasures chosen exceeds power of code erasures=%d p=%d\n",
               nerrs, p);
        usage();
    }
    // for (i = 0; i < nerrs; i++)
    // {
    //     if (frag_err_list[i] >= m)
    //     {
    //         printf(" fragment %d not in range\n", frag_err_list[i]);
    //         usage();
    //     }
    // }

    printf("ec_simple_example:\n");

    // Allocate coding matrices
    encode_matrix = (u8 *)malloc(m * k);
    decode_matrix = (u8 *)malloc(m * k);
    invert_matrix = (u8 *)malloc(m * k);
    temp_matrix = (u8 *)malloc(m * k);
    g_tbls = (u8 *)malloc(k * p * 32);

    if (encode_matrix == NULL || decode_matrix == NULL || invert_matrix == NULL || temp_matrix == NULL || g_tbls == NULL)
    {
        printf("Test failure! Error with malloc\n");
        return -1;
    }

    // Allocate the src & parity buffers
    frag_ptrs = (unsigned char ***)malloc(stripesize * sizeof(unsigned char **));
    // int rr = posix_memalign((void**)&frag_ptrs, 64, stripesize * sizeof(unsigned char));
    // assert(rr == 0);
    for (int i = 0; i < stripesize; i++)
    {
        frag_ptrs[i] = (unsigned char **)malloc(m * sizeof(unsigned char *));
        // int rr = posix_memalign((void**)&frag_ptrs[i], 64, m * sizeof(unsigned char));
        // assert(rr == 0);
        for (int j = 0; j < m; j++)
        {
            // frag_ptrs[i][j] = (unsigned char *)malloc(len * sizeof(unsigned char));
            int rr = posix_memalign((void**)&frag_ptrs[i][j], 64, len * sizeof(unsigned char));
            assert(rr == 0);
            // unsigned char *buf = (unsigned char *)malloc(4096);
        }
    }

    // cout << "a" << endl;

    // for (i = 0; i < stripesize; i++)
    // {
    //     for (size_t j = 0; j < m; j++)
    //     {
    //         if (NULL == (frag_ptrs[i][j] = (u8 *)malloc(len)))
    //         {
    //             printf("alloc error: Fail\n");
    //             return -1;
    //         }
    //     }
    // }

    // cout << "b" << endl;

    // Fill sources with random data
    for (uint32_t s = 0; s < stripesize; s++)
        for (i = 0; i < k; i++)
            for (j = 0; j < len; j++)
                frag_ptrs[s][i][j] = rand();
                // frag_ptrs[s][i][j] = 1;

    // cout << "c" << endl;

    // Pick an encode matrix. A Cauchy matrix is a good choice as even
    // large k are always invertable keeping the recovery rule simple.
    // gf_gen_cauchy1_matrix(encode_matrix, m, k);

    gf_gen_rs_matrix(encode_matrix, m, k);
    // gf_gen_cauchy1_matrix(encode_matrix, m, k);

    // Initialize g_tbls from encode matrix
    ec_init_tables(k, p, &encode_matrix[k * k], g_tbls);

    printf("encode (m,k,p)=(%d,%d,%d) len=%d\n", m, k, p, len);

    // Encode data using multiple threads
    double start_time = chrono::high_resolution_clock::now().time_since_epoch().count();

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int stripes = stripesize / NUM_THREADS;
        int offset_stripe = i * stripes;
        threads.push_back(std::thread(encode_data, frag_ptrs, encode_matrix, g_tbls, k, p, len, offset_stripe, stripes));

        // Get the native handle of the thread
        pthread_t native_handle = threads.back().native_handle();

        // cpu_set_t cpuset;
        // CPU_ZERO(&cpuset);
        // CPU_SET(i, &cpuset);

        // // Set the CPU affinity of the thread to the CPU set
        // int rc = pthread_setaffinity_np(native_handle, sizeof(cpu_set_t), &cpuset);
        // if (rc != 0)
        // {
        //     std::cerr << "Error setting CPU affinity: " << rc << std::endl;
        // }
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }

    double end_time = chrono::high_resolution_clock::now().time_since_epoch().count();
    double encode_bandwidth = (data_size * LOOP) / ((end_time - start_time) / 1000000000.0) / 1000000000.0;
    cout << "Encoding Time: " << ((end_time - start_time) / 1000000000.0) << " seconds" << endl;
    cout << "Encoding Data: " << (data_size * LOOP) / (1024 * 1024 * 1024LL) << " GB" << endl;
    cout << "Encoding bandwidth: " << encode_bandwidth << " GB/s" << endl;

    for (int i = 0; i < STRIPEMAX; i++)
    {
        for (int j = 0; j < MMAX; j++)
        {
            free(frag_ptrs[i][j]);
        }
        free(frag_ptrs[i]);
    }
    free(frag_ptrs);

    return 0;
}

/*
 * Generate decode matrix from encode matrix and erasure list
 *
 */

static int gf_gen_decode_matrix_simple(u8 *encode_matrix,
                                       u8 *decode_matrix,
                                       u8 *invert_matrix,
                                       u8 *temp_matrix,
                                       u8 *decode_index, u8 *frag_err_list, int nerrs, int k,
                                       int m)
{
    int i, j, p, r;
    int nsrcerrs = 0;
    u8 s, *b = temp_matrix;
    u8 frag_in_err[MMAX];

    memset(frag_in_err, 0, sizeof(frag_in_err));

    // Order the fragments in erasure for easier sorting
    for (i = 0; i < nerrs; i++)
    {
        if (frag_err_list[i] < k)
            nsrcerrs++;
        frag_in_err[frag_err_list[i]] = 1;
    }

    // Construct b (matrix that encoded remaining frags) by removing erased rows
    for (i = 0, r = 0; i < k; i++, r++)
    {
        while (frag_in_err[r])
            r++;
        for (j = 0; j < k; j++)
            b[k * i + j] = encode_matrix[k * r + j];
        decode_index[i] = r;
    }

    // Invert matrix to get recovery matrix
    if (gf_invert_matrix(b, invert_matrix, k) < 0)
        return -1;

    // Get decode matrix with only wanted recovery rows
    for (i = 0; i < nerrs; i++)
    {
        if (frag_err_list[i] < k) // A src err
            for (j = 0; j < k; j++)
                decode_matrix[k * i + j] =
                    invert_matrix[k * frag_err_list[i] + j];
    }

    // For non-src (parity) erasures need to multiply encode matrix * invert
    for (p = 0; p < nerrs; p++)
    {
        if (frag_err_list[p] >= k)
        { // A parity err
            for (i = 0; i < k; i++)
            {
                s = 0;
                for (j = 0; j < k; j++)
                    s ^= gf_mul(invert_matrix[j * k + i],
                                encode_matrix[k * frag_err_list[p] + j]);
                decode_matrix[k * p + i] = s;
            }
        }
    }
    return 0;
}