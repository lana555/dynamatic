#include <math.h>

// Adapted from:
// R. Kastner, J. Matai, and S. Neuendorffer.
// Parallel programming for FPGAs.

void fft(float X_R[100], float X_I[100], float a) {
    float temp_R; // temporary storage complex variable
    float temp_I; // temporary storage complex variable

    float c = cos(a);
    float s = sin(a);

    temp_R = X_R[8] * c - X_I[8] * s;
    temp_I = X_I[8] * c + X_R[8] * s;
    X_R[8] = X_R[0] - temp_R;
    X_I[8] = X_I[0] - temp_I;
    X_R[0] = X_R[0] + temp_R;
    X_I[0] = X_I[0] + temp_I;

    temp_R = X_R[9] * c - X_I[9] * s;
    temp_I = X_I[9] * c + X_R[9] * s;
    X_R[9] = X_R[1] - temp_R;
    X_I[9] = X_I[1] - temp_I;
    X_R[1] = X_R[1] + temp_R;
    X_I[1] = X_I[1] + temp_I;

    temp_R  = X_R[10] * c - X_I[10] * s;
    temp_I  = X_I[10] * c + X_R[10] * s;
    X_R[10] = X_R[2] - temp_R;
    X_I[10] = X_I[2] - temp_I;
    X_R[2]  = X_R[2] + temp_R;
    X_I[2]  = X_I[2] + temp_I;

    temp_R  = X_R[11] * c - X_I[11] * s;
    temp_I  = X_I[11] * c + X_R[11] * s;
    X_R[11] = X_R[3] - temp_R;
    X_I[11] = X_I[3] - temp_I;
    X_R[3]  = X_R[3] + temp_R;
    X_I[3]  = X_I[3] + temp_I;

    temp_R  = X_R[12] * c - X_I[12] * s;
    temp_I  = X_I[12] * c + X_R[12] * s;
    X_R[12] = X_R[4] - temp_R;
    X_I[12] = X_I[4] - temp_I;
    X_R[4]  = X_R[4] + temp_R;
    X_I[4]  = X_I[4] + temp_I;

    temp_R  = X_R[13] * c - X_I[13] * s;
    temp_I  = X_I[13] * c + X_R[13] * s;
    X_R[13] = X_R[5] - temp_R;
    X_I[13] = X_I[5] - temp_I;
    X_R[5]  = X_R[5] + temp_R;
    X_I[5]  = X_I[5] + temp_I;

    temp_R  = X_R[14] * c - X_I[14] * s;
    temp_I  = X_I[14] * c + X_R[14] * s;
    X_R[14] = X_R[6] - temp_R;
    X_I[14] = X_I[6] - temp_I;
    X_R[6]  = X_R[6] + temp_R;
    X_I[6]  = X_I[6] + temp_I;

    temp_R  = X_R[15] * c - X_I[15] * s;
    temp_I  = X_I[15] * c + X_R[15] * s;
    X_R[15] = X_R[7] - temp_R;
    X_I[15] = X_I[7] - temp_I;
    X_R[7]  = X_R[7] + temp_R;
    X_I[7]  = X_I[7] + temp_I;
}
