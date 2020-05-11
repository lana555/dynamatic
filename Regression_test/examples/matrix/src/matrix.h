typedef int in_int_t;
typedef int out_int_t;

#define A_ROWS 30
#define A_COLS 30
#define B_ROWS 30
#define B_COLS 30

void 
//__attribute__ ((noinline))  
matrix (in_int_t in_a[A_ROWS][A_COLS], in_int_t in_b[A_COLS][B_COLS], out_int_t out_c[A_ROWS][B_COLS]);
