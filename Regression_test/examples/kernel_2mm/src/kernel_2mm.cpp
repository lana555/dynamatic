/**
 * 2mm.c: This file is part of the PolyBench/C 3.2 test suite.
 *
 *
 * Contact: Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://polybench.sourceforge.net
 */

#include "kernel_2mm.h"
#include <stdio.h>
#include <stdlib.h>

void kernel_2mm(in_int_t alpha, in_int_t beta, inout_int_t tmp[N][N], in_int_t A[N][N], in_int_t B[N][N], in_int_t C[N][N], inout_int_t D[N][N])
{
  int i, j, k;

  for (i = 0; i < NI; i++)
    for (j = 0; j < NJ; j++)
      {
	tmp[i][j] = 0;
	for (k = 0; k < NK; ++k)
	  tmp[i][j] += alpha * A[i][k] * B[k][j];
      }
  for (i = 0; i < NI; i++)
    for (j = 0; j < NL; j++)
      {
	D[i][j] *= beta;
	for (k = 0; k < NJ; ++k)
	  D[i][j] += tmp[i][k] * C[k][j];
  }
}



#define AMOUNT_OF_TEST 1

int main(void){
    in_int_t alpha[AMOUNT_OF_TEST];
    in_int_t beta[AMOUNT_OF_TEST];
    in_int_t tmp[AMOUNT_OF_TEST][N][N];
    in_int_t A[AMOUNT_OF_TEST][N][N];
    in_int_t B[AMOUNT_OF_TEST][N][N];
    in_int_t C[AMOUNT_OF_TEST][N][N];
    inout_int_t D[AMOUNT_OF_TEST][N][N];
    

    for(int i = 0; i < AMOUNT_OF_TEST; ++i){
        alpha[i] = rand();
        beta[i] = rand();
        for(int y = 0; y < N; ++y){
            for(int x = 0; x < N; ++x){
                A[i][y][x] = rand()%100;
                B[i][y][x] = rand()%100;
                C[i][y][x] = rand()%100;
                D[i][y][x] = rand()%100;
            }
        }
    }

	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		int i = 0;
        kernel_2mm(alpha[0], beta[0], tmp[0], A[0], B[0], C[0], D[0]);
	//}
	
}


