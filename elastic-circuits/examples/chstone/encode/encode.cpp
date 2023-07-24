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
/*************************************************************************/
/*                                                                       */
/*   SNU-RT Benchmark Suite for Worst Case Timing Analysis               */
/*   =====================================================               */
/*                              Collected and Modified by S.-S. Lim      */
/*                                           sslim@archi.snu.ac.kr       */
/*                                         Real-Time Research Group      */
/*                                        Seoul National University      */
/*                                                                       */
/*                                                                       */
/*        < Features > - restrictions for our experimental environment   */
/*                                                                       */
/*          1. Completely structured.                                    */
/*               - There are no unconditional jumps.                     */
/*               - There are no exit from loop bodies.                   */
/*                 (There are no 'break' or 'return' in loop bodies)     */
/*          2. No 'switch' statements.                                   */
/*          3. No 'do..while' statements.                                */
/*          4. Expressions are restricted.                               */
/*               - There are no multiple expressions joined by 'or',     */
/*                'and' operations.                                      */
/*          5. No library calls.                                         */
/*               - All the functions needed are implemented in the       */
/*                 source file.                                          */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  FILE: adpcm.c                                                        */
/*  SOURCE : C Algorithms for Real-Time DSP by P. M. Embree              */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     CCITT G.722 ADPCM (Adaptive Differential Pulse Code Modulation)   */
/*     algorithm.                                                        */
/*     16khz sample rate data is stored in the array test_data[SIZE].    */
/*     Results are stored in the array compressed[SIZE] and result[SIZE].*/
/*     Execution time is determined by the constant SIZE (default value  */
/*     is 2000).                                                         */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
struct globalvars
{
/* G722 C code */

/* variables for transimit quadrature mirror filter here */
int tqmf[24];

/* QMF filter coefficients:
scaled by a factor of 4 compared to G722 CCITT recomendation */
int h[24] = {
  12, -44, -44, 212, 48, -624, 128, 1448,
  -840, -3220, 3804, 15504, 15504, 3804, -3220, -840,
  1448, 128, -624, 48, 212, -44, -44, 12
};

int xl, xh;

/* variables for receive quadrature mirror filter here */
int accumc[11], accumd[11];

/* outputs of decode() */
int xout1, xout2;

int xs, xd;

/* variables for encoder (hi and lo) here */

int il, szl, spl, sl, el;

int qq4_code4_table[16] = {
  0, -20456, -12896, -8968, -6288, -4240, -2584, -1200,
  20456, 12896, 8968, 6288, 4240, 2584, 1200, 0
};

/* quantization table 31 long to make quantl look-up easier,
last entry is for mil=30 case when wd is max */
int quantpos[31] = {
  61, 60, 59, 58, 57, 56, 55, 54,
  53, 52, 51, 50, 49, 48, 47, 46,
  45, 44, 43, 42, 41, 40, 39, 38,
  37, 36, 35, 34, 33, 32, 32
};

/* quantization table 31 long to make quantl look-up easier,
last entry is for mil=30 case when wd is max */
int quantneg[31] = {
  63, 62, 31, 30, 29, 28, 27, 26,
  25, 24, 23, 22, 21, 20, 19, 18,
  17, 16, 15, 14, 13, 12, 11, 10,
  9, 8, 7, 6, 5, 4, 4
};

/* decision levels - pre-multiplied by 8, 0 to indicate end */
int decis_levl[30] = {
  280, 576, 880, 1200, 1520, 1864, 2208, 2584,
  2960, 3376, 3784, 4240, 4696, 5200, 5712, 6288,
  6864, 7520, 8184, 8968, 9752, 10712, 11664, 12896,
  14120, 15840, 17560, 20456, 23352, 32767
};


int qq6_code6_table[64] = {
  -136, -136, -136, -136, -24808, -21904, -19008, -16704,
  -14984, -13512, -12280, -11192, -10232, -9360, -8576, -7856,
  -7192, -6576, -6000, -5456, -4944, -4464, -4008, -3576,
  -3168, -2776, -2400, -2032, -1688, -1360, -1040, -728,
  24808, 21904, 19008, 16704, 14984, 13512, 12280, 11192,
  10232, 9360, 8576, 7856, 7192, 6576, 6000, 5456,
  4944, 4464, 4008, 3576, 3168, 2776, 2400, 2032,
  1688, 1360, 1040, 728, 432, 136, -432, -136
};

int delay_bpl[6];

int delay_dltx[6];

int wl_code_table[16] = {
  -60, 3042, 1198, 538, 334, 172, 58, -30,
  3042, 1198, 538, 334, 172, 58, -30, -60
};

int ilb_table[32] = {
  2048, 2093, 2139, 2186, 2233, 2282, 2332, 2383,
  2435, 2489, 2543, 2599, 2656, 2714, 2774, 2834,
  2896, 2960, 3025, 3091, 3158, 3228, 3298, 3371,
  3444, 3520, 3597, 3676, 3756, 3838, 3922, 4008
};

int nbl;      /* delay line */
int al1, al2;
int plt, plt1, plt2;
int dlt;
int rlt, rlt1, rlt2;


int detl;


int deth;
int sh;       /* this comes from adaptive predictor */
int eh;

int qq2_code2_table[4] = {
  -7408, -1616, 7408, 1616
};

int wh_code_table[4] = {
  798, -214, 798, -214
};


int dh, ih;
int nbh, szh;
int sph, ph, yh, rh;

int delay_dhx[6];

int delay_bph[6];

int ah1, ah2;
int ph1, ph2;
int rh1, rh2;

/* variables for decoder here */
int ilr, rl;
int dec_deth, dec_detl, dec_dlt;

int dec_del_bpl[6];

int dec_del_dltx[6];

int dec_plt, dec_plt1, dec_plt2;
int dec_szl, dec_spl, dec_sl;
int dec_rlt1, dec_rlt2, dec_rlt;
int dec_al1, dec_al2;
int dl;
int dec_nbl, dec_dh, dec_nbh;

/* variables used in filtez */
int dec_del_bph[6];

int dec_del_dhx[6];

int dec_szh;
/* variables used in filtep */
int dec_rh1, dec_rh2;
int dec_ah1, dec_ah2;
int dec_ph, dec_sph;

int dec_sh;

int dec_ph1, dec_ph2;
};




int encode (int, int, globalvars *var);


// int filtez (int *bpl, int *dlt, globalvars var);
// void upzero (int dlt, int *dlti, int *bli, globalvars var);
// int filtep (int rlt1, int al1, int rlt2, int al2, globalvars var);
// int quantl (int el, int detl, globalvars var);
// int logscl (int il, int nbl, globalvars var);
// int scalel (int nbl, int shift_constant, globalvars var);
// int uppol2 (int al1, int al2, int plt, int plt1, int plt2);
// int uppol1 (int al1, int apl2, int plt, int plt1);
// int logsch (int ih, int nbh);


int filtez (int bpl[], int dlt[]) __attribute__((always_inline));
void upzero (int dlt, int dlti[6], int bli[6]) __attribute__((always_inline));
int filtep (int rlt1, int al1, int rlt2, int al2) __attribute__((always_inline));
int quantl (int el, int detl, globalvars *var) __attribute__((always_inline));
int logscl (int il, int nbl, globalvars *var) __attribute__((always_inline));
int scalel (int nbl, int shift_constant, globalvars *var) __attribute__((always_inline));
int uppol2 (int al1, int al2, int plt, int plt1, int plt2)  __attribute__((always_inline));
int uppol1 (int al1, int apl2, int plt, int plt1) __attribute__((always_inline));
int logsch (int ih, int nbh, globalvars *var) __attribute__((always_inline));
int abs (int n) __attribute__((always_inline));


int
abs (int n)
{
  int m;

  if (n >= 0)
    m = n;
  else
    m = -n;
  return m;
}


/* filtez - compute predictor output signal (zero section) */
/* input: bpl1-6 and dlt1-6, output: szl */

int
filtez (int bpl[], int dlt[])
{
    int i;
    long int zl;
    zl = (long) (bpl[0]) * (dlt[0]);
    for (i = 1; i < 6; i++)
        zl += (long) (bpl[i]) * (dlt[i]);

    return ((int) (zl >> 14));  /* x2 here */
}

/* filtep - compute predictor output signal (pole section) */
/* input rlt1-2 and al1-2, output spl */

int
filtep (int rlt1, int al1, int rlt2, int al2)
{
  long int pl, pl2;
  pl = 2 * rlt1;
  pl = (long) al1 *pl;
  pl2 = 2 * rlt2;
  pl += (long) al2 *pl2;
  return ((int) (pl >> 15));
}

/* quantl - quantize the difference signal in the lower sub-band */
int
quantl (int el, int detl, globalvars *var)
{
  int ril, mil;
  long int wd, decis;

/* abs of difference signal */
  wd = abs (el);
/* determine mil based on decision levels and detl gain */
  for (mil = 0; mil < 30; mil++)
    {
      decis = (var->decis_levl[mil] * (long) detl) >> 15L;
      if (wd <= decis)
  break;
    }
/* if mil=30 then wd is less than all decision levels */
  int pos = var->quantpos[mil];
  int neg = var->quantneg[mil];
  if (el >= 0)
    ril = pos;
  else
    ril = neg;
  

  return (ril);
}

/* logscl - update log quantizer scale factor in lower sub-band */
/* note that nbl is passed and returned */

int
logscl (int il, int nbl, globalvars *var)
{
  long int wd;
  wd = ((long) nbl * 127L) >> 7L; /* leak factor 127/128 */
  nbl = (int) wd + var->wl_code_table[il >> 2];
  if (nbl < 0)
    nbl = 0;
  if (nbl > 18432)
    nbl = 18432;
  return (nbl);
}

/* scalel: compute quantizer scale factor in lower or upper sub-band*/

int
scalel (int nbl, int shift_constant, globalvars *var)
{
  int wd1, wd2, wd3;
  wd1 = (nbl >> 6) & 31;
  wd2 = nbl >> 11;
  wd3 = var->ilb_table[wd1] >> (shift_constant + 1 - wd2);
  return (wd3 << 3);
}

/* upzero - inputs: dlt, dlti[0-5], bli[0-5], outputs: updated bli[0-5] */
/* also implements delay of bli and update of dlti from dlt */

void
upzero (int dlt, int dlti[6], int bli[6])
{
  int i, wd2, wd3;
/*if dlt is zero, then no sum into bli */
  if (dlt == 0)
    {
      for (i = 0; i < 6; i++)
  {
    bli[i] = (int) ((255L * bli[i]) >> 8L); /* leak factor of 255/256 */
  }
    }
  else
    {
      for (i = 0; i < 6; i++)
  {
    if ((long) dlt * dlti[i] >= 0)
      wd2 = 128;
    else
      wd2 = -128;
    wd3 = (int) ((255L * bli[i]) >> 8L);  /* leak factor of 255/256 */
    bli[i] = wd2 + wd3;
  }
    }
/* implement delay line for dlt */
  dlti[5] = dlti[4];
  dlti[4] = dlti[3];
  dlti[3] = dlti[2];
  dlti[2] = dlti[1];
  dlti[1] = dlti[0];
  dlti[0] = dlt;
}

/* uppol2 - update second predictor coefficient (pole section) */
/* inputs: al1, al2, plt, plt1, plt2. outputs: apl2 */

int
uppol2 (int al1, int al2, int plt, int plt1, int plt2)
{
  long int wd2, wd4;
  int apl2;
  wd2 = 4L * (long) al1;
  if ((long) plt * plt1 >= 0L)
    wd2 = -wd2;     /* check same sign */
  wd2 = wd2 >> 7;   /* gain of 1/128 */
  if ((long) plt * plt2 >= 0L)
    {
      wd4 = wd2 + 128;    /* same sign case */
    }
  else
    {
      wd4 = wd2 - 128;
    }
  apl2 = wd4 + (127L * (long) al2 >> 7L); /* leak factor of 127/128 */

/* apl2 is limited to +-.75 */
  if (apl2 > 12288)
    apl2 = 12288;
  if (apl2 < -12288)
    apl2 = -12288;
  return (apl2);
}

/* uppol1 - update first predictor coefficient (pole section) */
/* inputs: al1, apl2, plt, plt1. outputs: apl1 */

int
uppol1 (int al1, int apl2, int plt, int plt1)
{
  long int wd2;
  int wd3, apl1;
  wd2 = ((long) al1 * 255L) >> 8L;  /* leak factor of 255/256 */
  if ((long) plt * plt1 >= 0L)
    {
      apl1 = (int) wd2 + 192; /* same sign case */
    }
  else
    {
      apl1 = (int) wd2 - 192;
    }
/* note: wd3= .9375-.75 is always positive */
  wd3 = 15360 - apl2;   /* limit value */
  if (apl1 > wd3)
    apl1 = wd3;
  if (apl1 < -wd3)
    apl1 = -wd3;
  return (apl1);
}

/* logsch - update log quantizer scale factor in higher sub-band */
/* note that nbh is passed and returned */

int
logsch (int ih, int nbh, globalvars *var)
{
  int wd;
  wd = ((long) nbh * 127L) >> 7L; /* leak factor 127/128 */
  nbh = wd + var->wh_code_table[ih];
  if (nbh < 0)
    nbh = 0;
  if (nbh > 22528)
    nbh = 22528;
  return (nbh);
}




/* G722 encode function two ints in, one 8 bit output */

/* put input samples in xin1 = first value, xin2 = second value */
/* returns il and ih stored together */

int
encode (int xin1, int xin2, globalvars *var)
{
  int i;
  long int xa, xb;
  int decis;


/* main multiply accumulate loop for samples and coefficients */
  for (i = 0; i < 24; i+=2)
    {
        xa += (long) (var->tqmf[i]) * (var->h[i]);
        xb += (long) (var->tqmf[i+1]) * (var->h[i+1]);
    }


/* update delay line tqmf */
    for (i = 23; i >= 2; i--)
        var->tqmf[i] = var->tqmf[i-2];

/* scale outputs */
  var->xl = (xa + xb) >> 15;
  var->xh = (xa - xb) >> 15;

/* end of quadrature mirror filter code */

/* starting with lower sub band encoder */

/* filtez - compute predictor output section - zero section */
  var->szl = filtez (var->delay_bpl, var->delay_dltx);

/* filtep - compute predictor output signal (pole section) */
  var->spl = filtep (var->rlt1, var->al1, var->rlt2, var->al2);

/* compute the predictor output value in the lower sub_band encoder */
  var->sl = var->szl + var->spl;
  var->el = var->xl - var->sl;

/* quantl: quantize the difference signal */
  var->il = quantl (var->el, var->detl, var);

/* computes quantized difference signal */
/* for invqbl, truncate by 2 lsbs, so mode = 3 */
  var->dlt = ((long) var->detl * var->qq4_code4_table[var->il >> 2]) >> 15;

/* logscl: updates logarithmic quant. scale factor in low sub band */
  var->nbl = logscl (var->il, var->nbl, var);

/* scalel: compute the quantizer scale factor in the lower sub band */
/* calling parameters nbl and 8 (constant such that scalel can be scaleh) */
  var->detl = scalel (var->nbl, 8, var);

/* parrec - simple addition to compute recontructed signal for adaptive pred */
  var->plt = var->dlt + var->szl;

/* upzero: update zero section predictor coefficients (sixth order)*/
/* calling parameters: dlt, dlt1, dlt2, ..., dlt6 from dlt */
/*  bpli (linear_buffer in which all six values are delayed */
/* return params:      updated bpli, delayed dltx */
  upzero (var->dlt, var->delay_dltx, var->delay_bpl);

/* uppol2- update second predictor coefficient apl2 and delay it as al2 */
/* calling parameters: al1, al2, plt, plt1, plt2 */
  var->al2 = uppol2 (var->al1, var->al2, var->plt, var->plt1, var->plt2);

/* uppol1 :update first predictor coefficient apl1 and delay it as al1 */
/* calling parameters: al1, apl2, plt, plt1 */
  var->al1 = uppol1 (var->al1, var->al2, var->plt, var->plt1);

/* recons : compute recontructed signal for adaptive predictor */
  var->rlt = var->sl + var->dlt;

/* done with lower sub_band encoder; now implement delays for next time*/
  var->rlt2 = var->rlt1;
  var->rlt1 = var->rlt;
  var->plt2 = var->plt1;
  var->plt1 = var->plt;

/* high band encode */

  var->szh = filtez (var->delay_bph, var->delay_dhx);

  var->sph = filtep (var->rh1, var->ah1, var->rh2, var->ah2);

/* predic: sh = sph + szh */
  var->sh = var->sph + var->szh;
/* subtra: eh = xh - sh */
  var->eh = var->xh - var->sh;

/* quanth - quantization of difference signal for higher sub-band */
/* quanth: in-place for speed params: eh, deth (has init. value) */
  if (var->eh >= 0)
    {
      var->ih = 3;     /* 2,3 are pos codes */
    }
  else
    {
      var->ih = 1;     /* 0,1 are neg codes */
    }
  decis = (564L * (long) var->deth) >> 12L;
  if (abs (var->eh) > decis)
    var->ih--;     /* mih = 2 case */

/* compute the quantized difference signal, higher sub-band*/
  var->dh = ((long) var->deth * var->qq2_code2_table[var->ih]) >> 15L;

/* logsch: update logarithmic quantizer scale factor in hi sub-band*/
  var->nbh = logsch (var->ih, var->nbh, var);

/* note : scalel and scaleh use same code, different parameters */
  var->deth = scalel (var->nbh, 10, var);

/* parrec - add pole predictor output to quantized diff. signal */
  var->ph = var->dh + var->szh;

/* upzero: update zero section predictor coefficients (sixth order) */
/* calling parameters: dh, dhi, bphi */
/* return params: updated bphi, delayed dhx */
  upzero (var->dh, var->delay_dhx, var->delay_bph);

/* uppol2: update second predictor coef aph2 and delay as ah2 */
/* calling params: ah1, ah2, ph, ph1, ph2 */
  var->ah2 = uppol2 (var->ah1, var->ah2, var->ph, var->ph1, var->ph2);

/* uppol1:  update first predictor coef. aph2 and delay it as ah1 */
  var->ah1 = uppol1 (var->ah1, var->ah2, var->ph, var->ph1);

/* recons for higher sub-band */
  var->yh = var->sh + var->dh;

/* done with higher sub-band encoder, now Delay for next time */
  var->rh2 = var->rh1;
  var->rh1 = var->yh;
  var->ph2 = var->ph1;
  var->ph1 = var->ph;

/* multiplex ih and il to get signals together */
  return (var->il | (var->ih << 6));
}
