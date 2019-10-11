//========================================================================
// N-Queens
//========================================================================

//#include "nqueens.h"

void nqueens(int array[], int n) {
    int valid = 1;
    int p;
    int q;

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {

            if (j == i + 1) {
                p = array[i];
            }

            q = array[j];

            if (q == p || q == p - (j - i) || q == p + (j - i)) {
                valid    = 0;
                array[0] = 0;
            }
        }
    }

    array[0] = valid;
}
