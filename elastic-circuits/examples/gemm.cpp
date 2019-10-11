/**
 * gemver.c: This file is part of the PolyBench/C 3.2 test suite.
 *
 *
 * Contact: Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://polybench.sourceforge.net
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define N 4000

void kernel_gemver(int alpha, int beta, int A[30][30], int B[30][30], int C[30][30]) {
    int i, j, k;

    for (i = 0; i < 30; i++)
        for (j = 0; j < 30; j++) {
            int tmp = C[i][j] * beta;
            for (k = 0; k < 30; ++k)
                tmp += alpha * A[i][k] * B[k][j];

            C[i][j] = tmp;
        }
}
