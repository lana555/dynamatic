#include "mul_example.h"


void mul_example(inout_int_t a[100]){
    for (int i = 0; i < 100; ++i){
    	int x = a[i];
       a[i] = x* x* x;
    }
}


#define AMOUNT_OF_TEST 1

int main(void){
    in_int_t a[AMOUNT_OF_TEST][100];
    in_int_t b[AMOUNT_OF_TEST][100];
    in_int_t c[AMOUNT_OF_TEST];
    
    for(int i = 0; i < AMOUNT_OF_TEST; ++i){
        c[i] = 3;
        for(int j = 0; j < 100; ++j){
            a[i][j] = j;
            b[i][j] = 99 - j;
        }
    }

	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
        mul_example(a[0]);
	}
	
}