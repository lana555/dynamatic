
#include <stdlib.h>
#include "insertion_sort.h"

#define AMOUNT_OF_TEST 1

void insertion_sort(inout_int_t A[1000], in_int_t n)
{
  if (n <= 1) return;

  for (int i = 1; i < n; ++i) {
  	 int x = A[i];
  	 int j = i;
  	 while (j > 0 && A[j-1] > x) {
  	 	A[j] = A[j-1];
  	 	--j;
  	 }
  	 A[j] = x;
  }
}

int main(void){
	inout_int_t a[AMOUNT_OF_TEST][1000];
	in_int_t n[AMOUNT_OF_TEST];
    
	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		n[i] = 30;
		for(int j = 0; j < 1000; ++j){
    			a[i][j] = rand()%10;
			}
		}

	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		int i = 0;
		insertion_sort(a[i], n[i]);
	//}
}



