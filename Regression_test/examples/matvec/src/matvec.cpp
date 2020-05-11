#include "matvec.h"
//------------------------------------------------------------------------
// MatVec
//------------------------------------------------------------------------


#include <stdlib.h>
#include "matvec.h"

#define AMOUNT_OF_TEST 1

int matvec (in_int_t M[30][30], in_int_t V[30], out_int_t Out[30]) {
	int i, j;
	int tmp = 0;

	for (i=0;i<30;i++) {
		tmp = 0;

		for (j=0;j<30;j++) {
			tmp += V[j]* M[i][j];
		}
		Out[i] = tmp;
	}

	return tmp;

}

int main(void){
	in_int_t M[AMOUNT_OF_TEST][30][30];
	in_int_t V[AMOUNT_OF_TEST][30];
	out_int_t Out[AMOUNT_OF_TEST][30];
    
	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
    	for(int y = 0; y < 30; ++y){
      		V[i][y] = rand()%100;
      		for(int x = 0; x < 30; ++x){
      			M[i][y][x] = rand()%100;
      		}
    	}
  	}

	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
  	int i = 0;
	matvec(M[i], V[i], Out[i]);
	//}
}





