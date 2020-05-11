
#include <stdlib.h>
#include "threshold.h"

void threshold(inout_int_t red[1000], inout_int_t green[1000], inout_int_t blue[1000], in_int_t th) {
	for (int i = 0; i < 1000; i++) {
		int sum = red[i] + green [i] + blue [i];

		if (sum <= th) {
			red[i] = 0;
			green [i] = 0;
			blue [i] = 0;

		}

	}

}

#define AMOUNT_OF_TEST 1

int main(void){
	inout_int_t red[AMOUNT_OF_TEST][1000];
	inout_int_t green[AMOUNT_OF_TEST][1000];
	inout_int_t blue[AMOUNT_OF_TEST][1000];
	inout_int_t th[AMOUNT_OF_TEST];

	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		th[i] = (rand() % 100);
		for(int j = 0; j < 1000; ++j){
			red[i][j] = (rand() % 100);
			green[i][j] = (rand() % 100);
			blue[i][j] = (rand() % 100);
		}
	}
    
	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		int i = 0;
		threshold(red[i], green[i], blue[i], th[i]);
	//}
}


