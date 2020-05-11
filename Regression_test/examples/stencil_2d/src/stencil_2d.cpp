
#include <stdlib.h>
#include "stencil_2d.h"

int stencil_2d (in_int_t orig[900], out_int_t sol[900], in_int_t filter[10]){
    int temp = 0;
int mul = 0;

for (int r=0; r < 28; r++) {
    for (int c=0; c< 28; c++) {
         temp = 0;
         for (int k1=0;k1<3;k1++){
            for (int k2=0;k2<3;k2++){
                mul = filter[k1*3 + k2] * orig[(r+k1)*30 + c+k2];
                temp += mul;
            }
        }
        sol[(r*30) + c] = temp;
       
    }
}

return temp;
}

#define AMOUNT_OF_TEST 1

int main(void){
	in_int_t orig[AMOUNT_OF_TEST][900];
	out_int_t sol[AMOUNT_OF_TEST][900];
	in_int_t filter[AMOUNT_OF_TEST][10];


	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		for(int j = 0; j < 900; ++j){
			orig[i][j] = rand() % 100; 
		}
        for(int j = 0; j < 10; ++j){
			filter[i][j] = rand() % 100; 
		}
	}
    
	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
	int i = 0;
	stencil_2d(orig[i], sol[i], filter[i]);
	//}
}



