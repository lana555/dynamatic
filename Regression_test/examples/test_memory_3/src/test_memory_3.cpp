#include <stdlib.h>
#include "test_memory_3.h"

#define AMOUNT_OF_TEST 1

void test_memory_3(inout_int_t a[4], in_int_t n) {

	for (int i = 0; i < n; i++) {
		if (a[i] == n) 
			a[i] = 0;
	}
}

int main(void){
	inout_int_t a[AMOUNT_OF_TEST][4];
	in_int_t n[AMOUNT_OF_TEST];

	srand(13);
	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		n[i] = 4;
		for(int j = 0; j < 4; ++j){
			a[i][j] = rand();
		}
	}
    
	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
	int i = 0; 
	test_memory_3(a[i], n[i]);
	//}
}

