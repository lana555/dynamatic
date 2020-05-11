typedef int in_int_t;
typedef int out_int_t;
typedef int inout_int_t;

#define NI 10
#define NJ 10
#define NK 10
#define NL 10
#define NM 10
#define N 10

void kernel_3mm(in_int_t A[N][N], in_int_t B[N][N], in_int_t C[N][N], in_int_t D[N][N], inout_int_t E[N][N], inout_int_t F[N][N], out_int_t G[N][N]);