

#include <stdlib.h>
#include "test_memory_2.h"

#define AMOUNT_OF_TEST 1

void test_memory_2(inout_int_t a[5], in_int_t n) {

	for (int i = 0; i < n; i++) {
		a[i] = a[i+1] + 5;
	}
}


int main(void){
	inout_int_t a[AMOUNT_OF_TEST][5];
	in_int_t n[AMOUNT_OF_TEST];

	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		n[i] = 4;
		for(int j = 0; j < 5; ++j){
			a[i][j] = rand()%10;
		}
	}
    
	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
	int i = 0;
	test_memory_2(a[i], n[i]);
	//}
}

