//------------------------------------------------------------------------
// Benchmark Name: weightedTanh.cpp
//
// This benchmark computes and accumulates the tanh functions of array data.
//
// by Jianyi Cheng, 08032019
//------------------------------------------------------------------------

int func(int input[10]) {

    int i, beta, accum = 0, result;

    for (i = 0; i < 10; i++) {
        beta = input[i];

        if (beta <= 10) {
            result = result / beta;
            accum += result;
        }
    }

    return accum;
}
