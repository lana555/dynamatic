#include "histogram.h"
//------------------------------------------------------------------------
// Histogram
//------------------------------------------------------------------------

//SEPARATOR_FOR_MAIN

#include <stdlib.h>
#include "histogram.h"



void histogram( in_int_t x[100], in_int_t w[100], inout_int_t hist[100]) 
{
  for (int i=0; i<100; i++) {
    hist[x[i]] = hist[x[i]] * w[i];
  }
}
