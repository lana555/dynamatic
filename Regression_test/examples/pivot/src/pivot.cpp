//------------------------------------------------------------------------
// Pivot
//------------------------------------------------------------------------

#include <stdlib.h>
#include "pivot.h"

#define AMOUNT_OF_TEST 1

void pivot (int x[1000], int a[1000], int n, int k) {
	int i;

	for (i=k+1; i<=n; ++i) {

		x[k] = x[k]-a[i]*x[i];
	}

}

int main(void){
	inout_int_t x[AMOUNT_OF_TEST][1000];
	in_int_t a[AMOUNT_OF_TEST][1000];


	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
    	for(int j = 0; j < 1000; ++j){
      		x[i][j] = rand()%100;
      		a[i][j] = rand()%100;
    	}
  	}

	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		int i = 0;
		pivot(x[i], a[i], 100, 2);
	//}
}
