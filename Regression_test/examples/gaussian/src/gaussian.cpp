
#include <stdlib.h>
#include "gaussian.h"

#define AMOUNT_OF_TEST 1

void gaussian (in_int_t c[20], inout_int_t A[20][20]) {

    for(int j=1; j<=18; j++)
    /* loop for the generation of upper triangular matrix*/

        { for(int i=j+1; i<=18;i++)

            {

                for(int k=1; k<=19; k++)

                    {

                    A[i][k]=A[i][k]-c[j]*A[j][k];

                    }


            }

        }
    }

int main(void){
	  in_int_t c[AMOUNT_OF_TEST][20];
	  in_int_t A[AMOUNT_OF_TEST][20][20];
    
	srand(13);
	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
    	for(int y = 0; y < 20; ++y){
            c[i][y] = 1; //rand()%20;
    	    for(int x = 0; x < 20; ++x){
			    A[i][y][x] = 1; //rand()%20;
            }
		}
	}

	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		int i = 0;
		gaussian(c[i], A[i]);
	//}
}
