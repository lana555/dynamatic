#include "histogram.h"
//------------------------------------------------------------------------
// Histogram
//------------------------------------------------------------------------

//SEPARATOR_FOR_MAIN

#include <stdlib.h>
#include "histogram.h"

#define AMOUNT_OF_TEST 1

int main(void){
      in_int_t feature[AMOUNT_OF_TEST][100];
      in_int_t weight[AMOUNT_OF_TEST][100];
      inout_int_t hist[AMOUNT_OF_TEST][100];
      in_int_t n[AMOUNT_OF_TEST];
    
  
    for(int i = 0; i < AMOUNT_OF_TEST; ++i){
    n[i] = 100;
    for(int j = 0; j < 100; ++j){
      feature[i][j] = rand() % 50;
      weight[i][j] = rand() % 10;
      hist[i][j] = rand() % 50;
    }
    feature[0][3] = 5;
    feature[0][4] = 5;

    feature[0][30] = 5;
    feature[0][31] = 5;
    }

    for(int i = 0; i < AMOUNT_OF_TEST; ++i){
        histogram(feature[i], weight[i], hist[i]);
    }
}

