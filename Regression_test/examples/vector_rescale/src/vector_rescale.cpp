
#include <stdlib.h>
#include "vector_rescale.h"

void vector_rescale(inout_int_t a[1000], in_int_t c) {
	for (int i = 0; i < 1000; i++) {
		a[i] = a[i] * c;
	}
}


#define AMOUNT_OF_TEST 1

int main(void){
	in_int_t a[AMOUNT_OF_TEST][1000];
	in_int_t c[AMOUNT_OF_TEST];

	srand(13);
	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		c[i] = rand() % 100;
		for(int j = 0; j < 1000; ++j){
			a[i][j] = rand() % 100;
	    }
    }
    
	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
    int i = 0;
	vector_rescale(a[i], c[i]);
	//}
}





