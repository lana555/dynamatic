
#include <math.h>

void kernel_covariance(int data[30][30], int symmat[30][30], int mean[30]) {
    int i, j, j1, j2;
    int float_n = 30;

    /* Determine mean of column vectors of input data matrix */
    for (j = 0; j < 30; j++) {
        int x = 0;
        for (i = 0; i < 30; i++)
            x += data[i][j];
        mean[j] = x / float_n;
    }

    /* Center the column vectors. */
    for (i = 0; i < 30; i++)
        for (j = 0; j < 30; j++)
            data[i][j] -= mean[j];

    /* Calculate the m * m covariance matrix. */
    for (j1 = 0; j1 < 30; j1++)
        for (j2 = j1; j2 < 30; j2++) {
            int x = 0;
            for (i = 0; i < 30; i++)
                x += data[i][j1] * data[i][j2];

            symmat[j1][j2] = x;
            symmat[j2][j1] = x;
        }
}
