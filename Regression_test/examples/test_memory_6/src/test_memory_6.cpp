#include <stdlib.h>
#include "test_memory_6.h"

#define AMOUNT_OF_TEST 1

void test_memory_6(inout_int_t a[4], in_int_t n) {
	int x;
	for (int i = 2; i < n; i++) {
		a[i] = a[i-1] + a[i-2] + 5;
		a[i-1] = 0;

	}
}


int main(void){
	inout_int_t a[AMOUNT_OF_TEST][4];
	in_int_t n[AMOUNT_OF_TEST];

	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		n[i] = 4;
		for(int j = 0; j < 4; ++j){
			a[i][j] = rand()%10;
		}
	}
    
	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
	int i = 0; 
	test_memory_6(a[i], n[i]);
	//}
}
