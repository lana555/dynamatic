#include <math.h>

// Adapted from:
// R. Kastner, J. Matai, and S. Neuendorffer.
// Parallel programming for FPGAs.

void dft(float sample_real[100], float sample_imag[100], float out_real[100], float out_imag[100]) {

    for (int i = 0; i < 100; i++) {

        // hold intermediate frequency domain results
        float temp_real = 0;
        float temp_imag = 0;

        // 2pi*i/N
        float i_f = (float)i;
        float w   = (float)0.0628 * i_f;
        float z   = (float)w * i_f;

        // calculate frequency samples
        for (int j = 0; j < 100; j++) {
            float c = cos(z);
            float s = sin(z);

            // multiply current phasor with input sample
            // keep running sum
            temp_real += sample_real[j] * c - sample_imag[j] * s;
            temp_imag += sample_real[j] * s + sample_imag[j] * c;
        }
        out_real[i] = temp_real;
        out_imag[i] = temp_imag;
    }
}