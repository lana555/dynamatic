/**
 * atax.c: This file is part of the PolyBench/C 3.2 test suite.
 *
 *
 * Contact: Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://polybench.sourceforge.net
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NX 4000
#define NY 4000
#define N 4000

void kernel_atax(int A[N][N], int x[N], int y[N], int tmp[N]) {
    int i, j;

    for (i = 0; i < NY; i++)
        y[i] = 0;
    for (i = 0; i < NX; i++) {
        tmp[i] = 0;
        for (j = 0; j < NY; j++)
            tmp[i] = tmp[i] + A[i][j] * x[j];
        for (j = 0; j < NY; j++)
            y[j] = y[j] + A[i][j] * tmp[i];
    }
}
