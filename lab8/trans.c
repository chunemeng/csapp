/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 *
 * 522031910118 滕昊益
 */
#include "cachelab.h"
#include <stdio.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k_size, l_size, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

    // block store 8 int
    // with 32 set
    if (N == 32)
    {
        for (i = 0; i < N; i += 8)
        {
            for (j = 0; j < M; j += 8)
            {
                for (k_size = 0; k_size < 8; k_size++)
                {
                    tmp0 = A[i + k_size][j];
                    tmp1 = A[i + k_size][j + 1];
                    tmp2 = A[i + k_size][j + 2];
                    tmp3 = A[i + k_size][j + 3];
                    tmp4 = A[i + k_size][j + 4];
                    tmp5 = A[i + k_size][j + 5];
                    tmp6 = A[i + k_size][j + 6];
                    tmp7 = A[i + k_size][j + 7];

                    B[j][i + k_size] = tmp0;
                    B[j + 1][i + k_size] = tmp1;
                    B[j + 2][i + k_size] = tmp2;
                    B[j + 3][i + k_size] = tmp3;
                    B[j + 4][i + k_size] = tmp4;
                    B[j + 5][i + k_size] = tmp5;
                    B[j + 6][i + k_size] = tmp6;
                    B[j + 7][i + k_size] = tmp7;
                }
            }
        }
    }
    else
    {
        if (N == 64)
        {
            for (i = 0; i < 64; i += 8)
            {
                for (j = 0; j < 64; j += 8)
                {
                    for (k_size = 0; k_size < 4; k_size++)
                    {
                        tmp0 = A[i + k_size][j];
                        tmp1 = A[i + k_size][j + 1];
                        tmp2 = A[i + k_size][j + 2];
                        tmp3 = A[i + k_size][j + 3];
                        tmp4 = A[i + k_size][j + 4];
                        tmp5 = A[i + k_size][j + 5];
                        tmp6 = A[i + k_size][j + 6];
                        tmp7 = A[i + k_size][j + 7];

                        B[j][i + k_size] = tmp0;
                        B[j + 1][i + k_size] = tmp1;
                        B[j + 2][i + k_size] = tmp2;
                        B[j + 3][i + k_size] = tmp3;

                        B[j][i + k_size + 4] = tmp4;
                        B[j + 1][i + k_size + 4] = tmp5;
                        B[j + 2][i + k_size + 4] = tmp6;
                        B[j + 3][i + k_size + 4] = tmp7;
                    }
                    for (k_size = 0; k_size < 4; k_size++)
                    {
                        tmp0 = B[k_size + j][i + 4];
                        tmp1 = B[k_size + j][i + 5];
                        tmp2 = B[k_size + j][i + 6];
                        tmp3 = B[k_size + j][i + 7];

                        tmp4 = A[i + 4][j + k_size];
                        tmp5 = A[i + 5][j + k_size];
                        tmp6 = A[i + 6][j + k_size];
                        tmp7 = A[i + 7][j + k_size];

                        B[j + k_size][i + 4] = tmp4;
                        B[j + k_size][i + 5] = tmp5;
                        B[j + k_size][i + 6] = tmp6;
                        B[j + k_size][i + 7] = tmp7;

                        B[j + k_size + 4][i + 0] = tmp0;
                        B[j + k_size + 4][i + 1] = tmp1;
                        B[j + k_size + 4][i + 2] = tmp2;
                        B[j + k_size + 4][i + 3] = tmp3;
                    }
                    for (k_size = 0; k_size < 4; ++k_size)
                    {
                        tmp0 = A[i + 4][j + k_size + 4];
                        tmp1 = A[i + 5][j + k_size + 4];
                        tmp2 = A[i + 6][j + k_size + 4];
                        tmp3 = A[i + 7][j + k_size + 4];

                        B[j + k_size + 4][i + 4] = tmp0;
                        B[j + k_size + 4][i + 5] = tmp1;
                        B[j + k_size + 4][i + 6] = tmp2;
                        B[j + k_size + 4][i + 7] = tmp3;
                    }
                }
            }
        }
        else
        {
            for (i = 0; i < N; i += 17)
            {
                for (j = 0; j < M; j += 17)
                {
                    for (k_size = 0; k_size < (N - i >= 17 ? 17 : N - i); k_size++)
                    {
                        for (l_size = 0; l_size < (M - j >= 17 ? 17 : M - j); l_size++)
                        {
                            B[j + l_size][i + k_size] = A[i + k_size][j + l_size];
                        }
                    }
                }
            }
        }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
