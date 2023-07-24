//------------------------------------------------------------------------
// FIR
//------------------------------------------------------------------------

int compute(int a, int b, int c) { return a * b + c; }

int fir(int d_i[1000]) {

    int i;
    int tmp = 0;
    int idx[1000];
For_Loop:
    for (i = 0; i < 1000; i++) {
        // tmp += idx [i] * d_i[999-i];
        tmp += compute(idx[i], d_i[999 - i], i);
    }

    return tmp;
}
