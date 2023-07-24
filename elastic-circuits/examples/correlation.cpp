#include <math.h>
/* Main computational kernel. The whole function will be timed,
   including the call and return. */
void kernel_correlation(float data[30][30], float symmat[30][30], float mean[30], float stdev[30]) {
    int i, j, j1, j2;
    float float_n = 30.0;
    float eps     = 0.1f;

    /* Determine mean of column vectors of input data matrix */
    for (j = 0; j < 30; j++) {
        float x = 0.0;
        for (i = 0; i < 30; i++)
            x += data[i][j];
        mean[j] = x / float_n;
    }

    /* Determine standard deviations of column vectors of data matrix. */
    for (j = 0; j < 30; j++) {
        float x = 0.0;
        for (i = 0; i < 30; i++)
            x += (data[i][j] - mean[j]) * (data[i][j] - mean[j]);
        x /= float_n;
        x = x * x; // = sqrt(x); replacing in vhdl

        /* The following in an inelegant but usual way to handle
     near-zero std. dev. values, which below would cause a zero-
     divide. */
        stdev[j] = x <= eps ? 1.0 : x;
    }

    /* Center and reduce the column vectors. */
    for (i = 0; i < 30; i++)
        for (j = 0; j < 30; j++) {
            float x = data[i][j];
            x -= mean[j];
            float n = float_n * float_n; // sqrt(float_n)
            x /= n * stdev[j];
            data[i][j] = x;
        }

    /* Calculate the m * m correlation matrix. */
    for (j1 = 0; j1 < 29; j1++) {
        symmat[j1][j1] = 1.0;
        for (j2 = j1 + 1; j2 < 30; j2++) {
            float x = 0.0;
            for (i = 0; i < 30; i++)
                x += (data[i][j1] * data[i][j2]);
            symmat[j1][j2] = x;
            symmat[j2][j1] = x;
        }
    }
    symmat[29][29] = 1.0;
}
