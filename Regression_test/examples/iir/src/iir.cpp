#include "iir.h"
//------------------------------------------------------------------------
// IIR
//------------------------------------------------------------------------


#include <stdlib.h>
#include "iir.h"

#define AMOUNT_OF_TEST 1

int iir (in_int_t y[1000], in_int_t x[1000], in_int_t a, in_int_t b) {
	int i;
	int tmp=y[0];

	for (i=1;i<1000;i++) {
		tmp = a*tmp + b*x[i];
		y[i] = tmp;

	}
	return tmp;

}

int main(void){
	in_int_t y[AMOUNT_OF_TEST][1000];
	in_int_t x[AMOUNT_OF_TEST][1000];
	in_int_t b[AMOUNT_OF_TEST];
	in_int_t a[AMOUNT_OF_TEST];
    
	srand(13);
	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		a[i] = rand();
		b[i] = rand();
		for(int j = 0; j < 1000; ++j){
    		y[i][j] = rand()%1000;
    		x[i][j] = rand()%1000;
		}
	}

	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		int i = 0;
		iir(y[i], x[i], a[i], b[i]);
	//}
}





