#include "triangular.h"
//------------------------------------------------------------------------
// Triangular
//------------------------------------------------------------------------

void triangular( in_int_t x[10], inout_int_t A[10][10] , in_int_t n) {

    for (int i=n-1; i>=0; i--) {
       // x[i] = A[i][n]/A[i][i];
	    for (int k=i-1;k>=0; k--) {
            A[k][n] -= A[k][i] * x[i];
        }
	}
}

#include <stdlib.h>

#define AMOUNT_OF_TEST 1

int main(void){
	in_int_t xArray[AMOUNT_OF_TEST][10];
	in_int_t A[AMOUNT_OF_TEST][10][10];
	in_int_t n[AMOUNT_OF_TEST];

	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		n[i] = 10; //(rand() % 100);
		for(int x = 0; x < 10; ++x){
            xArray[i][x] = rand() % 100;
		    for(int y = 0; y < 10; ++y){
			    A[i][y][x] = rand() % 100;
		    }
	    }
    }
    
	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
    int i = 0;
	triangular(xArray[i], A[i], n[i]);
	//}
}




