
/**
 * jacobi-1d-imper.c: This file is part of the PolyBench/C 3.2 test suite.
 *
 *
 * Contact: Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://polybench.sourceforge.net
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TSTEPS 100
#define N 10000

void kernel_jacobi_1d_imper(int A[N], int B[N]) {
    int t, i, j;

    for (t = 0; t < TSTEPS; t++) {
        for (i = 1; i < N - 1; i++)
            B[i] = 3 * (A[i - 1] + A[i] + A[i + 1]);
        for (j = 1; j < N - 1; j++)
            A[j] = B[j];
    }
}
