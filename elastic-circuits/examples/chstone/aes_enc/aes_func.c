/*
+--------------------------------------------------------------------------+
| CHStone : a suite of benchmark programs for C-based High-Level Synthesis |
| ======================================================================== |
|                                                                          |
| * Collected and Modified : Y. Hara, H. Tomiyama, S. Honda,               |
|                            H. Takada and K. Ishii                        |
|                            Nagoya University, Japan                      |
|                                                                          |
| * Remark :                                                               |
|    1. This source code is modified to unify the formats of the benchmark |
|       programs in CHStone.                                               |
|    2. Test vectors are added for CHStone.                                |
|    3. If "main_result" is 0 at the end of the program, the program is    |
|       correctly executed.                                                |
|    4. Please follow the copyright of each benchmark program.             |
+--------------------------------------------------------------------------+
*/
/* aes_function.c */
/*
 * Copyright (C) 2005
 * Akira Iwata & Masayuki Sato
 * Akira Iwata Laboratory,
 * Nagoya Institute of Technology in Japan.
 *
 * All rights reserved.
 *
 * This software is written by Masayuki Sato.
 * And if you want to contact us, send an email to Kimitake Wakayama
 * (wakayama@elcom.nitech.ac.jp)
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must var->retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software must
 *    display the following acknowledgment:
 *    "This product includes software developed by Akira Iwata Laboratory,
 *    Nagoya Institute of Technology in Japan (http://mars.elcom.nitech.ac.jp/)."
 *
 * 4. Redistributions of any form whatsoever must var->retain the following
 *    acknowledgment:
 *    "This product includes software developed by Akira Iwata Laboratory,
 *     Nagoya Institute of Technology in Japan (http://mars.elcom.nitech.ac.jp/)."
 *
 *   THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *   AKIRA IWATA LABORATORY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 *   SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 *   IN NO EVENT SHALL AKIRA IWATA LABORATORY BE LIABLE FOR ANY SPECIAL,
 *   INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 *   FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 *   NEGLIGENCE OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION
 *   WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* ********* ByteSub & ShiftRow ********* */
void
ByteSub_ShiftRow (int statemt[32], int nb, globalvars *var)
{
  int temp;

  switch (nb)
    {
    case 4:
      temp = var->Sbox[statemt[1] >> 4][statemt[1] & 0xf];
      statemt[1] = var->Sbox[statemt[5] >> 4][statemt[5] & 0xf];
      statemt[5] = var->Sbox[statemt[9] >> 4][statemt[9] & 0xf];
      statemt[9] = var->Sbox[statemt[13] >> 4][statemt[13] & 0xf];
      statemt[13] = temp;

      temp = var->Sbox[statemt[2] >> 4][statemt[2] & 0xf];
      statemt[2] = var->Sbox[statemt[10] >> 4][statemt[10] & 0xf];
      statemt[10] = temp;
      temp = var->Sbox[statemt[6] >> 4][statemt[6] & 0xf];
      statemt[6] = var->Sbox[statemt[14] >> 4][statemt[14] & 0xf];
      statemt[14] = temp;

      temp = var->Sbox[statemt[3] >> 4][statemt[3] & 0xf];
      statemt[3] = var->Sbox[statemt[15] >> 4][statemt[15] & 0xf];
      statemt[15] = var->Sbox[statemt[11] >> 4][statemt[11] & 0xf];
      statemt[11] = var->Sbox[statemt[7] >> 4][statemt[7] & 0xf];
      statemt[7] = temp;

      statemt[0] = var->Sbox[statemt[0] >> 4][statemt[0] & 0xf];
      statemt[4] = var->Sbox[statemt[4] >> 4][statemt[4] & 0xf];
      statemt[8] = var->Sbox[statemt[8] >> 4][statemt[8] & 0xf];
      statemt[12] = var->Sbox[statemt[12] >> 4][statemt[12] & 0xf];
      break;
    case 6:
      temp = var->Sbox[statemt[1] >> 4][statemt[1] & 0xf];
      statemt[1] = var->Sbox[statemt[5] >> 4][statemt[5] & 0xf];
      statemt[5] = var->Sbox[statemt[9] >> 4][statemt[9] & 0xf];
      statemt[9] = var->Sbox[statemt[13] >> 4][statemt[13] & 0xf];
      statemt[13] = var->Sbox[statemt[17] >> 4][statemt[17] & 0xf];
      statemt[17] = var->Sbox[statemt[21] >> 4][statemt[21] & 0xf];
      statemt[21] = temp;

      temp = var->Sbox[statemt[2] >> 4][statemt[2] & 0xf];
      statemt[2] = var->Sbox[statemt[10] >> 4][statemt[10] & 0xf];
      statemt[10] = var->Sbox[statemt[18] >> 4][statemt[18] & 0xf];
      statemt[18] = temp;
      temp = var->Sbox[statemt[6] >> 4][statemt[6] & 0xf];
      statemt[6] = var->Sbox[statemt[14] >> 4][statemt[14] & 0xf];
      statemt[14] = var->Sbox[statemt[22] >> 4][statemt[22] & 0xf];
      statemt[22] = temp;

      temp = var->Sbox[statemt[3] >> 4][statemt[3] & 0xf];
      statemt[3] = var->Sbox[statemt[15] >> 4][statemt[15] & 0xf];
      statemt[15] = temp;
      temp = var->Sbox[statemt[7] >> 4][statemt[7] & 0xf];
      statemt[7] = var->Sbox[statemt[19] >> 4][statemt[19] & 0xf];
      statemt[19] = temp;
      temp = var->Sbox[statemt[11] >> 4][statemt[11] & 0xf];
      statemt[11] = var->Sbox[statemt[23] >> 4][statemt[23] & 0xf];
      statemt[23] = temp;

      statemt[0] = var->Sbox[statemt[0] >> 4][statemt[0] & 0xf];
      statemt[4] = var->Sbox[statemt[4] >> 4][statemt[4] & 0xf];
      statemt[8] = var->Sbox[statemt[8] >> 4][statemt[8] & 0xf];
      statemt[12] = var->Sbox[statemt[12] >> 4][statemt[12] & 0xf];
      statemt[16] = var->Sbox[statemt[16] >> 4][statemt[16] & 0xf];
      statemt[20] = var->Sbox[statemt[20] >> 4][statemt[20] & 0xf];
      break;
    case 8:
      temp = var->Sbox[statemt[1] >> 4][statemt[1] & 0xf];
      statemt[1] = var->Sbox[statemt[5] >> 4][statemt[5] & 0xf];
      statemt[5] = var->Sbox[statemt[9] >> 4][statemt[9] & 0xf];
      statemt[9] = var->Sbox[statemt[13] >> 4][statemt[13] & 0xf];
      statemt[13] = var->Sbox[statemt[17] >> 4][statemt[17] & 0xf];
      statemt[17] = var->Sbox[statemt[21] >> 4][statemt[21] & 0xf];
      statemt[21] = var->Sbox[statemt[25] >> 4][statemt[25] & 0xf];
      statemt[25] = var->Sbox[statemt[29] >> 4][statemt[29] & 0xf];
      statemt[29] = temp;

      temp = var->Sbox[statemt[2] >> 4][statemt[2] & 0xf];
      statemt[2] = var->Sbox[statemt[14] >> 4][statemt[14] & 0xf];
      statemt[14] = var->Sbox[statemt[26] >> 4][statemt[26] & 0xf];
      statemt[26] = var->Sbox[statemt[6] >> 4][statemt[6] & 0xf];
      statemt[6] = var->Sbox[statemt[18] >> 4][statemt[18] & 0xf];
      statemt[18] = var->Sbox[statemt[30] >> 4][statemt[30] & 0xf];
      statemt[30] = var->Sbox[statemt[10] >> 4][statemt[10] & 0xf];
      statemt[10] = var->Sbox[statemt[22] >> 4][statemt[22] & 0xf];
      statemt[22] = temp;

      temp = var->Sbox[statemt[3] >> 4][statemt[3] & 0xf];
      statemt[3] = var->Sbox[statemt[19] >> 4][statemt[19] & 0xf];
      statemt[19] = temp;
      temp = var->Sbox[statemt[7] >> 4][statemt[7] & 0xf];
      statemt[7] = var->Sbox[statemt[23] >> 4][statemt[23] & 0xf];
      statemt[23] = temp;
      temp = var->Sbox[statemt[11] >> 4][statemt[11] & 0xf];
      statemt[11] = var->Sbox[statemt[27] >> 4][statemt[27] & 0xf];
      statemt[27] = temp;
      temp = var->Sbox[statemt[15] >> 4][statemt[15] & 0xf];
      statemt[15] = var->Sbox[statemt[31] >> 4][statemt[31] & 0xf];
      statemt[31] = temp;

      statemt[0] = var->Sbox[statemt[0] >> 4][statemt[0] & 0xf];
      statemt[4] = var->Sbox[statemt[4] >> 4][statemt[4] & 0xf];
      statemt[8] = var->Sbox[statemt[8] >> 4][statemt[8] & 0xf];
      statemt[12] = var->Sbox[statemt[12] >> 4][statemt[12] & 0xf];
      statemt[16] = var->Sbox[statemt[16] >> 4][statemt[16] & 0xf];
      statemt[20] = var->Sbox[statemt[20] >> 4][statemt[20] & 0xf];
      statemt[24] = var->Sbox[statemt[24] >> 4][statemt[24] & 0xf];
      statemt[28] = var->Sbox[statemt[28] >> 4][statemt[28] & 0xf];
      break;
    }
}

int
SubByte (int in, globalvars *var)
{
  return var->Sbox[(in / 16)][(in % 16)];
}

/* ********* InversShiftRow & ByteSub ********* */
// void
// InversShiftRow_ByteSub (int statemt[32], int nb, globalvars *var)
// {
//   int temp;

//   switch (nb)
//     {
//     case 4:
//       temp = var->invSbox[statemt[13] >> 4][statemt[13] & 0xf];
//       statemt[13] = var->invSbox[statemt[9] >> 4][statemt[9] & 0xf];
//       statemt[9] = var->invSbox[statemt[5] >> 4][statemt[5] & 0xf];
//       statemt[5] = var->invSbox[statemt[1] >> 4][statemt[1] & 0xf];
//       statemt[1] = temp;

//       temp = var->invSbox[statemt[14] >> 4][statemt[14] & 0xf];
//       statemt[14] = var->invSbox[statemt[6] >> 4][statemt[6] & 0xf];
//       statemt[6] = temp;
//       temp = var->invSbox[statemt[2] >> 4][statemt[2] & 0xf];
//       statemt[2] = var->invSbox[statemt[10] >> 4][statemt[10] & 0xf];
//       statemt[10] = temp;

//       temp = var->invSbox[statemt[15] >> 4][statemt[15] & 0xf];
//       statemt[15] = var->invSbox[statemt[3] >> 4][statemt[3] & 0xf];
//       statemt[3] = var->invSbox[statemt[7] >> 4][statemt[7] & 0xf];
//       statemt[7] = var->invSbox[statemt[11] >> 4][statemt[11] & 0xf];
//       statemt[11] = temp;

//       statemt[0] = var->invSbox[statemt[0] >> 4][statemt[0] & 0xf];
//       statemt[4] = var->invSbox[statemt[4] >> 4][statemt[4] & 0xf];
//       statemt[8] = var->invSbox[statemt[8] >> 4][statemt[8] & 0xf];
//       statemt[12] = var->invSbox[statemt[12] >> 4][statemt[12] & 0xf];
//       break;
//     case 6:
//       temp = var->invSbox[statemt[21] >> 4][statemt[21] & 0xf];
//       statemt[21] = var->invSbox[statemt[17] >> 4][statemt[17] & 0xf];
//       statemt[17] = var->invSbox[statemt[13] >> 4][statemt[13] & 0xf];
//       statemt[13] = var->invSbox[statemt[9] >> 4][statemt[9] & 0xf];
//       statemt[9] = var->invSbox[statemt[5] >> 4][statemt[5] & 0xf];
//       statemt[5] = var->invSbox[statemt[1] >> 4][statemt[1] & 0xf];
//       statemt[1] = temp;

//       temp = var->invSbox[statemt[22] >> 4][statemt[22] & 0xf];
//       statemt[22] = var->invSbox[statemt[14] >> 4][statemt[14] & 0xf];
//       statemt[14] = var->invSbox[statemt[6] >> 4][statemt[6] & 0xf];
//       statemt[6] = temp;
//       temp = var->invSbox[statemt[18] >> 4][statemt[18] & 0xf];
//       statemt[18] = var->invSbox[statemt[10] >> 4][statemt[10] & 0xf];
//       statemt[10] = var->invSbox[statemt[2] >> 4][statemt[2] & 0xf];
//       statemt[2] = temp;

//       temp = var->invSbox[statemt[15] >> 4][statemt[15] & 0xf];
//       statemt[15] = var->invSbox[statemt[3] >> 4][statemt[3] & 0xf];
//       statemt[3] = temp;
//       temp = var->invSbox[statemt[19] >> 4][statemt[19] & 0xf];
//       statemt[19] = var->invSbox[statemt[7] >> 4][statemt[7] & 0xf];
//       statemt[7] = temp;
//       temp = var->invSbox[statemt[23] >> 4][statemt[23] & 0xf];
//       statemt[23] = var->invSbox[statemt[11] >> 4][statemt[11] & 0xf];
//       statemt[11] = temp;

//       statemt[0] = var->invSbox[statemt[0] >> 4][statemt[0] & 0xf];
//       statemt[4] = var->invSbox[statemt[4] >> 4][statemt[4] & 0xf];
//       statemt[8] = var->invSbox[statemt[8] >> 4][statemt[8] & 0xf];
//       statemt[12] = var->invSbox[statemt[12] >> 4][statemt[12] & 0xf];
//       statemt[16] = var->invSbox[statemt[16] >> 4][statemt[16] & 0xf];
//       statemt[20] = var->invSbox[statemt[20] >> 4][statemt[20] & 0xf];
//       break;
//     case 8:
//       temp = var->invSbox[statemt[29] >> 4][statemt[29] & 0xf];
//       statemt[29] = var->invSbox[statemt[25] >> 4][statemt[25] & 0xf];
//       statemt[25] = var->invSbox[statemt[21] >> 4][statemt[21] & 0xf];
//       statemt[21] = var->invSbox[statemt[17] >> 4][statemt[17] & 0xf];
//       statemt[17] = var->invSbox[statemt[13] >> 4][statemt[13] & 0xf];
//       statemt[13] = var->invSbox[statemt[9] >> 4][statemt[9] & 0xf];
//       statemt[9] = var->invSbox[statemt[5] >> 4][statemt[5] & 0xf];
//       statemt[5] = var->invSbox[statemt[1] >> 4][statemt[1] & 0xf];
//       statemt[1] = temp;

//       temp = var->invSbox[statemt[30] >> 4][statemt[30] & 0xf];
//       statemt[30] = var->invSbox[statemt[18] >> 4][statemt[18] & 0xf];
//       statemt[18] = var->invSbox[statemt[6] >> 4][statemt[6] & 0xf];
//       statemt[6] = var->invSbox[statemt[26] >> 4][statemt[26] & 0xf];
//       statemt[26] = var->invSbox[statemt[14] >> 4][statemt[14] & 0xf];
//       statemt[14] = var->invSbox[statemt[2] >> 4][statemt[2] & 0xf];
//       statemt[2] = var->invSbox[statemt[22] >> 4][statemt[22] & 0xf];
//       statemt[22] = var->invSbox[statemt[10] >> 4][statemt[10] & 0xf];
//       statemt[10] = temp;

//       temp = var->invSbox[statemt[31] >> 4][statemt[31] & 0xf];
//       statemt[31] = var->invSbox[statemt[15] >> 4][statemt[15] & 0xf];
//       statemt[15] = temp;
//       temp = var->invSbox[statemt[27] >> 4][statemt[27] & 0xf];
//       statemt[27] = var->invSbox[statemt[11] >> 4][statemt[11] & 0xf];
//       statemt[11] = temp;
//       temp = var->invSbox[statemt[23] >> 4][statemt[23] & 0xf];
//       statemt[23] = var->invSbox[statemt[7] >> 4][statemt[7] & 0xf];
//       statemt[7] = temp;
//       temp = var->invSbox[statemt[19] >> 4][statemt[19] & 0xf];
//       statemt[19] = var->invSbox[statemt[3] >> 4][statemt[3] & 0xf];
//       statemt[3] = temp;

//       statemt[0] = var->invSbox[statemt[0] >> 4][statemt[0] & 0xf];
//       statemt[4] = var->invSbox[statemt[4] >> 4][statemt[4] & 0xf];
//       statemt[8] = var->invSbox[statemt[8] >> 4][statemt[8] & 0xf];
//       statemt[12] = var->invSbox[statemt[12] >> 4][statemt[12] & 0xf];
//       statemt[16] = var->invSbox[statemt[16] >> 4][statemt[16] & 0xf];
//       statemt[20] = var->invSbox[statemt[20] >> 4][statemt[20] & 0xf];
//       statemt[24] = var->invSbox[statemt[24] >> 4][statemt[24] & 0xf];
//       statemt[28] = var->invSbox[statemt[28] >> 4][statemt[28] & 0xf];
//       break;
//     }
// }

/* ******** MixColumn ********** */
void
MixColumn_AddRoundKey (int statemt[32], int nb, int n, globalvars *var)
{
  int j;
  register int x;
  for(int i = 0; i < 8*4; i++)
    var->ret[i] = 0;



  for (j = 0; j < nb; ++j)
    {
      var->ret[j * 4] = (statemt[j * 4] << 1);
      if ((var->ret[j * 4] >> 8) == 1)
	var->ret[j * 4] ^= 283;
      x = statemt[1 + j * 4];
      x ^= (x << 1);
      if ((x >> 8) == 1)
	var->ret[j * 4] ^= (x ^ 283);
      else
	var->ret[j * 4] ^= x;
      var->ret[j * 4] ^=
	statemt[2 + j * 4] ^ statemt[3 + j * 4] ^ var->word[0][j + nb * n];

      var->ret[1 + j * 4] = (statemt[1 + j * 4] << 1);
      if ((var->ret[1 + j * 4] >> 8) == 1)
	var->ret[1 + j * 4] ^= 283;
      x = statemt[2 + j * 4];
      x ^= (x << 1);
      if ((x >> 8) == 1)
	var->ret[1 + j * 4] ^= (x ^ 283);
      else
	var->ret[1 + j * 4] ^= x;
      var->ret[1 + j * 4] ^=
	statemt[3 + j * 4] ^ statemt[j * 4] ^ var->word[1][j + nb * n];

      var->ret[2 + j * 4] = (statemt[2 + j * 4] << 1);
      if ((var->ret[2 + j * 4] >> 8) == 1)
	var->ret[2 + j * 4] ^= 283;
      x = statemt[3 + j * 4];
      x ^= (x << 1);
      if ((x >> 8) == 1)
	var->ret[2 + j * 4] ^= (x ^ 283);
      else
	var->ret[2 + j * 4] ^= x;
      var->ret[2 + j * 4] ^=
	statemt[j * 4] ^ statemt[1 + j * 4] ^ var->word[2][j + nb * n];

      var->ret[3 + j * 4] = (statemt[3 + j * 4] << 1);
      if ((var->ret[3 + j * 4] >> 8) == 1)
	var->ret[3 + j * 4] ^= 283;
      x = statemt[j * 4];
      x ^= (x << 1);
      if ((x >> 8) == 1)
	var->ret[3 + j * 4] ^= (x ^ 283);
      else
	var->ret[3 + j * 4] ^= x;
      var->ret[3 + j * 4] ^=
	statemt[1 + j * 4] ^ statemt[2 + j * 4] ^ var->word[3][j + nb * n];
    }
  for (j = 0; j < nb; ++j)
    {
      statemt[j * 4] = var->ret[j * 4];
      statemt[1 + j * 4] = var->ret[1 + j * 4];
      statemt[2 + j * 4] = var->ret[2 + j * 4];
      statemt[3 + j * 4] = var->ret[3 + j * 4];
    }
  // return 0;
}

/* ******** InversMixColumn ********** */
// int
// AddRoundKey_InversMixColumn (int statemt[32], int nb, int n, globalvars *var)
// {
//   int i, j;
//   register int x;
//   for(int i = 0; i < 8*4; i++)
//       var->ret[i] = 0;


//   for (j = 0; j < nb; ++j)
//     {
//       statemt[j * 4] ^= var->word[0][j + nb * n];
//       statemt[1 + j * 4] ^= var->word[1][j + nb * n];
//       statemt[2 + j * 4] ^= var->word[2][j + nb * n];
//       statemt[3 + j * 4] ^= var->word[3][j + nb * n];
//     }
//   for (j = 0; j < nb; ++j)
//     for (i = 0; i < 4; ++i)
//       {
// 	x = (statemt[i + j * 4] << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x ^= statemt[i + j * 4];
// 	x = (x << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x ^= statemt[i + j * 4];
// 	x = (x << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	var->ret[i + j * 4] = x;

// 	x = (statemt[(i + 1) % 4 + j * 4] << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x = (x << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x ^= statemt[(i + 1) % 4 + j * 4];
// 	x = (x << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x ^= statemt[(i + 1) % 4 + j * 4];
// 	var->ret[i + j * 4] ^= x;

// 	x = (statemt[(i + 2) % 4 + j * 4] << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x ^= statemt[(i + 2) % 4 + j * 4];
// 	x = (x << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x = (x << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x ^= statemt[(i + 2) % 4 + j * 4];
// 	var->ret[i + j * 4] ^= x;

// 	x = (statemt[(i + 3) % 4 + j * 4] << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x = (x << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x = (x << 1);
// 	if ((x >> 8) == 1)
// 	  x ^= 283;
// 	x ^= statemt[(i + 3) % 4 + j * 4];
// 	var->ret[i + j * 4] ^= x;
//       }
//   for (i = 0; i < nb; ++i)
//     {
//       statemt[i * 4] = var->ret[i * 4];
//       statemt[1 + i * 4] = var->ret[1 + i * 4];
//       statemt[2 + i * 4] = var->ret[2 + i * 4];
//       statemt[3 + i * 4] = var->ret[3 + i * 4];
//     }
//   return 0;
// }

/* ******** AddRoundKey ********** */
void
AddRoundKey (int statemt[32], int type, int n, globalvars *var)
{
  int j;

  switch (type)
    {
    case 128128:
    case 192128:
    case 256128:
      var->nb = 4;
      break;
    case 128192:
    case 192192:
    case 256192:
      var->nb = 6;
      break;
    case 128256:
    case 192256:
    case 256256:
      var->nb = 8;
      break;
    }
  for (j = 0; j < var->nb; ++j)
    {
      statemt[j * 4] ^= var->word[0][j + var->nb * n];
      statemt[1 + j * 4] ^= var->word[1][j + var->nb * n];
      statemt[2 + j * 4] ^= var->word[2][j + var->nb * n];
      statemt[3 + j * 4] ^= var->word[3][j + var->nb * n];
    }
}
