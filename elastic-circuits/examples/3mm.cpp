/**
 * 3mm.c: This file is part of the PolyBench/C 3.2 test suite.
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
#define NM 1024
#define N 1024

void kernel_3mm(int A[N][N], int B[N][N], int C[N][N], int D[N][N], int E[N][N], int F[N][N],
                int G[N][N])

{
    int i, j, k;

    for (i = 0; i < NI; i++)
        for (j = 0; j < NJ; j++) {
            E[i][j] = 0;
            for (k = 0; k < NK; ++k)
                E[i][j] += A[i][k] * B[k][j];
        }

    for (i = 0; i < NJ; i++)
        for (j = 0; j < NL; j++) {
            F[i][j] = 0;
            for (k = 0; k < NM; ++k)
                F[i][j] += C[i][k] * D[k][j];
        }
    for (i = 0; i < NI; i++)
        for (j = 0; j < NL; j++) {
            G[i][j] = 0;
            for (k = 0; k < NJ; ++k)
                G[i][j] += E[i][k] * F[k][j];
        }
}
