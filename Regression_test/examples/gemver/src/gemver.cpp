#include "gemver.h"

/**
 * gemver.c: This file is part of the PolyBench/C 3.2 test suite.
 *
 *
 * Contact: Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://polybench.sourceforge.net
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define N 30


#include <stdlib.h>

void gemver(in_int_t alpha, in_int_t beta, inout_int_t A[N][N],
 in_int_t u1[N], in_int_t v1[N], in_int_t u2[N], in_int_t v2[N], inout_int_t w[N], inout_int_t x[N], in_int_t y[N], in_int_t z[N])
{
  int i, j;

for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
      A[i][j] = A[i][j] + u1[i] * v1[j] + u2[i] * v2[j];

  for (i = 0; i < N; i++) {
    int tmp = x[i];
    for (j = 0; j < N; j++)
      tmp = tmp + beta * A[j][i] * y[j];
    x[i] = tmp;
  }

  for (i = 0; i < N; i++)

    x[i] = x[i] + z[i];

  for (i = 0; i < N; i++) {
   int tmp = w[i];
    for (j = 0; j < N; j++)
      tmp = tmp +  alpha * A[i][j] * x[j];
    w[i] = tmp;
  }

}

#define AMOUNT_OF_TEST 1

int main(void){
	  in_int_t alpha[AMOUNT_OF_TEST];
	  in_int_t beta[AMOUNT_OF_TEST];
	  inout_int_t A[AMOUNT_OF_TEST][N][N];
	  in_int_t u1[AMOUNT_OF_TEST][N];
	  in_int_t v1[AMOUNT_OF_TEST][N];
	  in_int_t u2[AMOUNT_OF_TEST][N];
	  in_int_t v2[AMOUNT_OF_TEST][N];
	  inout_int_t w[AMOUNT_OF_TEST][N];
	  inout_int_t x[AMOUNT_OF_TEST][N];
	  in_int_t y[AMOUNT_OF_TEST][N];
	  in_int_t z[AMOUNT_OF_TEST][N];

  
	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
    alpha[i] = rand()% 20;
    beta[i] = rand()% 20;
    	for(int yy = 0; yy < N; ++yy){
        u1[i][yy] = rand()% 20;
        v1[i][yy] = rand()% 20;
        u2[i][yy] = rand()% 20;
        v2[i][yy] = rand()% 20;
        w[i][yy] = rand()% 20;
        x[i][yy] = rand()% 20;
        y[i][yy] = rand()% 20;
        z[i][yy] = rand()% 20;
    	    for(int x = 0; x < N; ++x){
			      A[i][yy][x] = rand()%10;
          }
		  }
	}

	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		int i = 0;
		gemver(alpha[i], beta[i], A[i], u1[i], v1[i], u2[i], v2[i], w[i], x[i], y[i], z[i]);
	//}
}





