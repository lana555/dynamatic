
#define ERROR (-1)

#define SEQUENCE_END_CODE       0x1B7

#define MV_FIELD 0

#define Num 2048

struct globalvars
{
  int i, j, k;
  int main_result;
  int PMV[2][2][2];
  int dmvector[2];
  int motion_vertical_field_select[2][2];
  int s, motion_vector_count, mv_format, h_r_size, v_r_size, dmv, mvscale;
  unsigned char inRdbfr[Num];
  unsigned char out_ld_Rdptr[Num];
  int System_Stream_Flag;
unsigned char ld_Rdbfr[2048];
unsigned int ld_Bfr;
int ld_Incnt;
unsigned int ptr_ld_rd, ld_Rdmax;
};

struct mvtab
{

const char MVtab0[8][2] = {
  {ERROR, 0}, {3, 3}, {2, 2}, {2, 2},
  {1, 1}, {1, 1}, {1, 1}, {1, 1}
};

/* Table B-10, motion_code, codes 0000011 ... 000011x */
const char MVtab1[8][2] = {
  {ERROR, 0}, {ERROR, 0}, {ERROR, 0}, {7, 6},
  {6, 6}, {5, 6}, {4, 5}, {4, 5}
};

/* Table B-10, motion_code, codes 0000001100 ... 000001011x */
const char MVtab2[12][2] = {
  {16, 9}, {15, 9}, {14, 9}, {13, 9}, {12, 9}, {11, 9},
  {10, 8}, {10, 8}, {9, 8}, {9, 8}, {8, 8}, {8, 8}
};

};


int read (unsigned char s1[Num], const unsigned char s2[Num], int n) __attribute__((always_inline));
void Fill_Buffer (globalvars *var) __attribute__((always_inline));
void Flush_Buffer ( int N, globalvars *var) __attribute__((always_inline));
unsigned int Show_Bits (int N, globalvars *var) __attribute__((always_inline));
unsigned int Get_Bits ( int N, globalvars *var) __attribute__((always_inline));
unsigned int Get_Bits1 (globalvars *var) __attribute__((always_inline));
int Get_motion_code (globalvars *var, mvtab *mv);
void decode_motion_vector (int *pred, int r_size, int motion_code, int motion_residual, int full_pel_vector) __attribute__((always_inline));
int Get_dmvector (globalvars *var) __attribute__((always_inline));
void
motion_vector (
     int PMV[2],
     int dmvector[],
     int h_r_size,
     int v_r_size,
     int dmv,   /* MPEG-2 only: get differential motion vectors */
     int mvscale,   /* MPEG-2 only: field vector in frame pic */
     int full_pel_vector  /* MPEG-1 only */,
     globalvars *var, mvtab *mv);





int read (unsigned char s1[Num], const unsigned char s2[Num], int n)
{
  // unsigned char *p1;
  // const unsigned char *p2;
  int n_tmp;
  int i = 0;
// p1 = s1;
//   p2 = s2;
  n_tmp = n;
  
while (n_tmp-- > 0)
    {
      s1[i] = s2[i];
      i++;
}
  
return n;
}


void Fill_Buffer (globalvars *var)
{
  int Buffer_Level;
  // unsigned char *p;
  // p = var.ld_Rdbfr;


  Buffer_Level = read (var->ld_Rdbfr, var->inRdbfr, 2048);
  var->ptr_ld_rd = 0;
 //  // var.ld_Rdptr = var.ld_Rdbfr;

  if (var->System_Stream_Flag)
    var->ld_Rdmax -= 2048;


  /* end of the bitstream file */
  if (Buffer_Level < 2048)
    {
      /* just to be safe */
      if (Buffer_Level < 0)
  Buffer_Level = 0;

      /* pad until the next to the next 32-bit word boundary */
      while (Buffer_Level & 3)
  var->ld_Rdbfr[Buffer_Level++] = 0;

      /* pad the buffer with sequence end codes */
      while (Buffer_Level < 2048)
  {
    var->ld_Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE >> 24;
    var->ld_Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE >> 16;
    var->ld_Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE >> 8;
    var->ld_Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE & 0xff;
  }
    }
}



void
Flush_Buffer ( int N, globalvars *var)
{
  int Incnt;
  int i = 0;

#ifdef RAND_VAL 
  /* N is between 0 and 20 with realistic input sets, while it may become larger than the width of the integer type when using randomly generated input sets which are used in the contained input set. The following is to avoid this.  */
  var->ld_Bfr <<= (N%20);
#else
  var->ld_Bfr <<= N;
#endif
  
  Incnt = var->ld_Incnt -= N;

  if (Incnt <= 24)
    {
      if (var->ptr_ld_rd < 2044)
  {
    do
      {
#ifdef RAND_VAL 
  /* N is between 0 and 20 with realistic input sets, while it may become larger than the width of the integer type when using randomly generated input sets which are used in the contained input set. The following is to avoid this.  */
        var->ld_Bfr |= var->ld_Rdbfr[(var->ptr_ld_rd)++] << ((24 - Incnt)%20);
#else
        var->ld_Bfr |= var->ld_Rdbfr[(var->ptr_ld_rd)++] << (24 - Incnt);
#endif
        Incnt += 8;
      }
    while (Incnt <= 24);
  }
      else
  {
    do
      {
        if (var->ptr_ld_rd >= 2048)
      Fill_Buffer (var);
#ifdef RAND_VAL 
  /* N is between 0 and 20 with realistic input sets, while it may become larger than the width of the integer type when using randomly generated input sets which are used in the contained input set. The following is to avoid this.  */
        var->ld_Bfr |= var->ld_Rdbfr[(var->ptr_ld_rd)++] << ((24 - Incnt)%20);
#else
        var->ld_Bfr |= var->ld_Rdbfr[(var->ptr_ld_rd)++] << (24 - Incnt);
#endif
        Incnt += 8;
      }
    while (Incnt <= 24);
  }
      var->ld_Incnt = Incnt;
    }
}




unsigned int Show_Bits (int N, globalvars *var)
{
  return var->ld_Bfr >> (unsigned)(32-N)%32;
}



unsigned int
Get_Bits ( int N, globalvars *var)
{
  unsigned int Val;

  Val = Show_Bits (N, var);
  Flush_Buffer (N, var);

  return Val;
}




unsigned int
Get_Bits1 (globalvars *var)
{
  return Get_Bits (1, var);
}


int
Get_motion_code (globalvars *var, mvtab *mv)
{
  int code;
  int mvtab_val0, mvtab_val1;
  // int temp_code = 0;

  if (Get_Bits1 (var))
    {
      return 0;
    }
code = Show_Bits (9, var);


for(int i =0; i<1; i++)
{
  if(i == 0)
  {
        if(code >= 64)
    {
        code >>= 6;
        // temp_code = code;
        mvtab_val0 = mv->MVtab0[code][i];
        mvtab_val1 = mv->MVtab0[code][i+1];
    }
    else if(code >= 24)
    {
        code >>=3;
        // temp_code = code;
        mvtab_val0 = mv->MVtab1[code][i];
        mvtab_val1 = mv->MVtab1[code][i+1];
    }
    else if((code -= 12) < 0)
        return 0;
    else
    {
        mvtab_val0 = mv->MVtab2[code][i];
        mvtab_val1 = mv->MVtab2[code][i+1];
    }


  }

}


  Flush_Buffer (mvtab_val1, var);
  return Get_Bits1 (var)? -mvtab_val0 : mvtab_val0;
}

void
decode_motion_vector (
     int *pred,
     int r_size, int motion_code, int motion_residual,
     int full_pel_vector  /* MPEG-1 (ISO/IEC 11172-1) support */ )
{
  int lim, vec;

  r_size = r_size % 32;
  lim = 16 << r_size;
  vec = full_pel_vector ? (*pred >> 1) : (*pred);

  if (motion_code > 0)
    {
      vec += ((motion_code - 1) << r_size) + motion_residual + 1;
      if (vec >= lim)
  vec -= lim + lim;
    }
  else if (motion_code < 0)
    {
      vec -= ((-motion_code - 1) << r_size) + motion_residual + 1;
      if (vec < -lim)
  vec += lim + lim;
    }
  *pred = full_pel_vector ? (vec << 1) : vec;
}


int
Get_dmvector (globalvars *var)
{
  unsigned int temp = Get_Bits (1, var);
  if (temp)
    {
      return temp ? -1 : 1;
    }
  else
    {
      return 0;
    }
}




void
motion_vector (
     int PMV[2],
     int dmvector[],
     int h_r_size,
     int v_r_size,
     int dmv,   /* MPEG-2 only: get differential motion vectors */
     int mvscale,   /* MPEG-2 only: field vector in frame pic */
     int full_pel_vector  /* MPEG-1 only */,
     globalvars *var, mvtab *mv)
{
  int motion_code;
  int motion_residual;

  /* horizontal component */
  /* ISO/IEC 13818-2 Table B-10 */
  motion_code = Get_motion_code (var, mv);

  motion_residual = (h_r_size != 0
         && motion_code != 0) ? Get_Bits (h_r_size, var) : 0;

/*Passing &PMV[0] to the function is the problem here*/
  decode_motion_vector (&PMV[0], h_r_size, motion_code, motion_residual, full_pel_vector);

  if (dmv)
    dmvector[0] = Get_dmvector (var);


  /* vertical component */
  // motion_code = Get_motion_code (var, mv);
  // motion_residual = (v_r_size != 0 && motion_code != 0) ? Get_Bits (v_r_size, var) : 0;

  // if (mvscale)
  //   PMV[1] >>= 1;    /* DIV 2 */

  // decode_motion_vector (&PMV[1], v_r_size, motion_code, motion_residual,
  //    full_pel_vector);

  // if (mvscale)
  //   PMV[1] <<= 1;

  // if (dmv)
  //   dmvector[1] = Get_dmvector (var);

}