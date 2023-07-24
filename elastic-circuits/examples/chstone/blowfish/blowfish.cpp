#define N 40
#define KEYSIZE 5200


#include "blowfish.h"
#include "bf_locl.h"
#include "bf_pi.h"
#include "bf_skey.c"
#include "bf_cfb64.c"
#include "bf_enc.c"

/*
+--------------------------------------------------------------------------+
| * Test Vectors (added for CHStone)                                       |
|     in_key : input data                                                  |
|     out_key : expected output data                                       |
+--------------------------------------------------------------------------+
*/






void
blowfish ( unsigned char indata[N], unsigned char outdata[N], globalvars *var, unsigned char ukey[8], unsigned char ivec[8])
{
  
  // int num;
  int i, j, k, l;
  int encordec;
  int check;

  var->num = 0;
  k = 0;
  l = 0;
  encordec = 1;
  check = 0;
  for (i = 0; i < 8; i++)
    {
      ukey[i] = 0;
      ivec[i] = 0;
    }
  BF_set_key (8, ukey, var);
  i = 0;
  while (k < KEYSIZE)
    {
      while (k < KEYSIZE && i < N)
	indata[i++] = var->in_key[k++];

      BF_cfb64_encrypt (indata, outdata, i, ivec, encordec, var);

 // //      for (j = 0; j < i; j++)
	// // check += (outdata[j] != out_key[l++]);

      i = 0;
    }
}
