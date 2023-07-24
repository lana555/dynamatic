#include <math.h>

// Adapted from:
// R. Kastner, J. Matai, and S. Neuendorffer.
// Parallel programming for FPGAs.

void dft(float sample_real[1000], float sample_imag[1000], float out_real[1000],
         float out_imag[1000], float w) {

    float c = cos(w);
    float s = sin(w);

    float temp_real_0 = sample_real[0] * c - sample_imag[0] * s;
    float temp_imag_0 = sample_real[0] * s + sample_imag[0] * c;

    float temp_real_1 = sample_real[1] * c - sample_imag[1] * s;
    float temp_imag_1 = sample_real[1] * s + sample_imag[1] * c;

    float temp_real_2 = sample_real[2] * c - sample_imag[2] * s;
    float temp_imag_2 = sample_real[2] * s + sample_imag[2] * c;

    float temp_real_3 = sample_real[3] * c - sample_imag[3] * s;
    float temp_imag_3 = sample_real[3] * s + sample_imag[3] * c;

    float temp_real_4 = sample_real[4] * c - sample_imag[4] * s;
    float temp_imag_4 = sample_real[4] * s + sample_imag[4] * c;

    float temp_real_5 = sample_real[5] * c - sample_imag[5] * s;
    float temp_imag_5 = sample_real[5] * s + sample_imag[5] * c;

    float temp_real_6 = sample_real[6] * c - sample_imag[6] * s;
    float temp_imag_6 = sample_real[6] * s + sample_imag[6] * c;

    float temp_real_7 = sample_real[7] * c - sample_imag[7] * s;
    float temp_imag_7 = sample_real[7] * s + sample_imag[7] * c;

    out_real[0] = temp_real_0 + temp_real_1 + temp_real_2 + temp_real_3 + temp_real_4 +
                  temp_real_5 + temp_real_6 + temp_real_7;
    out_imag[0] = temp_imag_0 + temp_imag_1 + temp_imag_2 + temp_imag_3 + temp_imag_4 +
                  temp_imag_5 + temp_imag_6 + temp_imag_7;
}