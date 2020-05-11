


#include <stdlib.h>
#include "sumi3_mem.h"

#define AMOUNT_OF_TEST 1

int sumi3_mem(in_int_t a[1000])
{
	int sum = 0;
	for (int i = 0; i < 1000; i++) {
		int x = a[i];
		sum += x*x*x;
	}
	return sum;
}

int main(void){
	in_int_t a[AMOUNT_OF_TEST][1000];

	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		for(int j = 0; j < 1000; ++j){
			a[i][j] = rand() % 10;
		}
	}
    
	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
	int i = 0;
	sumi3_mem(a[i]);
	//}
}


