#include <math.h>

void complexdiv(int a_i[1000], int a_r[1000], int b_i[1000], int b_r[1000], int c_i[1000],
                int c_r[1000]) {
    int i;

    for (i = 0; i < 1000; i++) {
        int bi = b_i[i];
        int br = b_r[i];
        int ai = a_i[i];
        int ar = a_r[i];
        int cr, ci;
        if (abs(br) >= abs(bi)) {
            int r   = bi / br;
            int den = br + r * bi;
            cr      = (ar + r * ai) / den;
            ci      = (ai - r * ar) / den;
        } else {
            int r   = br / bi;
            int den = bi + r * br;
            cr      = (ar * r + ai) / den;
            ci      = (ai * r - ar) / den;
        }
        c_r[i] = cr;
        c_i[i] = ci;
    }
}