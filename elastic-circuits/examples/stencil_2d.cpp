int stencil_2d(int orig[900], int sol[900], int filter[10]) {
    int temp = 0;
    int mul  = 0;

    for (int r = 0; r < 28; r++) {
        for (int c = 0; c < 28; c++) {
            temp = 0;
            for (int k1 = 0; k1 < 3; k1++) {
                for (int k2 = 0; k2 < 3; k2++) {
                    mul = filter[k1 * 3 + k2] * orig[(r + k1) * 30 + c + k2];
                    temp += mul;
                }
            }
            sol[(r * 30) + c] = temp;
        }
    }

    return temp;
}