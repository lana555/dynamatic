/**
 * 2mm.c: This file is part of the PolyBench/C 3.2 test suite.
 *
 *
 * Contact: Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://polybench.sourceforge.net
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NI 1024
#define NJ 1024
#define NK 1024
#define NL 1024
#define N 1024

void kernel_2mm(int alpha, int beta, int tmp[N][N], int A[N][N], int B[N][N], int C[N][N],
                int D[N][N]) {
    int i, j, k;

    for (i = 0; i < NI; i++)
        for (j = 0; j < NJ; j++) {
            tmp[i][j] = 0;
            for (k = 0; k < NK; ++k)
                tmp[i][j] += alpha * A[i][k] * B[k][j];
        }
    for (i = 0; i < NI; i++)
        for (j = 0; j < NL; j++) {
            D[i][j] *= beta;
            for (k = 0; k < NJ; ++k)
                D[i][j] += tmp[i][k] * C[k][j];
        }
}
