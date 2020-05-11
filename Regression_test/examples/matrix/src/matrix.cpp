#include "matrix.h"

#define A_ROWS 30
#define A_COLS 30
#define B_ROWS 30
#define B_COLS 30

#include <stdlib.h>



// matrix multiplication of a A*B matrix
void 
//__attribute__ ((noinline))  
matrix (in_int_t in_a[A_ROWS][A_COLS], in_int_t in_b[A_COLS][B_COLS], out_int_t out_c[A_ROWS][B_COLS])
//void mm(int(* __restrict__ in_a)[A_COLS], int(* __restrict__ in_b)[B_COLS], int(* __restrict__ out_c)[B_COLS])
{
    int i,j,k;
    for (i = 0; i < A_ROWS; i++)
    {
        for (j = 0; j < B_COLS; j++)
        {
            int sum_mult = 0;
            for (k = 0; k < A_COLS; k++)
            {
                sum_mult += in_a[i][k] * in_b[k][j];
            }
            out_c[i][j] = sum_mult;
        }
    }
}


#define AMOUNT_OF_TEST 1

int main(void){
	in_int_t in_a[AMOUNT_OF_TEST][A_ROWS][B_ROWS];
	in_int_t in_b[AMOUNT_OF_TEST][A_ROWS][B_ROWS];
	in_int_t out_c[AMOUNT_OF_TEST][A_ROWS][B_ROWS];
    
	srand(13);
	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		for(int y = 0; y < A_ROWS; ++y){
            for(int x = 0; x < A_ROWS; ++x){
                in_a[i][y][x] = rand()%10;
                in_b[i][y][x] = rand()%10;
            }
        }
    }

	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
    int i = 0;
	matrix(in_a[i], in_b[i], out_c[i]);
	//}
}




