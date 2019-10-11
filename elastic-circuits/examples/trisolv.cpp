/**
 * trisolv.c: This file is part of the PolyBench/C 3.2 test suite.
 *
 *
 * Contact: Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://polybench.sourceforge.net
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define N 100000

void trisolv(int n, int x[N], int A[N][N], int c[N]) {
    int i, j;

    for (i = 0; i < N; i++) {
        x[i] = c[i];
        for (j = 0; j <= i - 1; j++)
            x[i] = x[i] - A[i][j] * x[j];
        x[i] = x[i] / A[i][i];
    }
}
