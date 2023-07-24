#include <math.h>

// Adapted from:
// R. Kastner, J. Matai, and S. Neuendorffer.
// Parallel programming for FPGAs.

void fft(float X_R[100], float X_I[100]) {
    float temp_R; // temporary storage complex variable
    float temp_I; // temporary storage complex variable
    int i, j, k;  // loop indexes
    int lower;    // Index of lower point in butterfly
    int step, stage, DFTpts;
    int numBF;   // Butterfly Width
    int N2 = 10; // N2=N>>1

    // bit reverse(X_R, X_I);
    step = N2;
    float a, e, c, s;

    for (stage = 1; stage <= 16; stage++) {

        DFTpts = 1 << stage; // DFT = 2ˆstage = points in sub DFT
        numBF  = DFTpts / 2; // Butterfly WIDTHS in sub−DFT
        k      = 0;
        e      = 1 / DFTpts;
        a      = 0.0;

        // Perform butterflies for j−th stage
        for (j = 0; j < 16; j++) {
            c = cos(a);
            s = sin(a);
            a = a + e;

            // Compute butterflies
            for (i = j; i < 16; i += DFTpts) {
                lower      = i + numBF; // index of lower point in butterfly
                temp_R     = X_R[lower] * c - X_I[lower] * s;
                temp_I     = X_I[lower] * c + X_R[lower] * s;
                X_R[lower] = X_R[i] - temp_R;
                X_I[lower] = X_I[i] - temp_I;
                X_R[i]     = X_R[i] + temp_R;
                X_I[i]     = X_I[i] + temp_I;
            }
            k += step;
        }
        step = step / 2;
    }
}
