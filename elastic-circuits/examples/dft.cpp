int dft(int cos[1000], int sin[1000], int sample_real[1000], int sample_imag[1000],
        int out_real[1000], int out_imag[1000]) {

    int temp_real = 0;
    int temp_imag = 0;

    for (int i = 0; i < 1000; i++) {
        temp_real = 0;
        temp_imag = 0;

        for (int j = 0; j < 1000; j++) {
            int c = cos[i];
            int s = sin[i];

            temp_real += sample_real[j] * c - sample_imag[j] * s;
            temp_imag += sample_real[j] * s + sample_imag[j] * c;
        }
        out_real[i] = temp_real;
        out_imag[i] = temp_imag;
    }
    return temp_imag + temp_real;
}