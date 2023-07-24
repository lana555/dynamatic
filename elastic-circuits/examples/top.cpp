int top(int input[10], int ref[10]) {
    int i, tmp, res = 0, k, j, tmp2, tmp3, res2, reg;
    int li, ltmp, lj, ltmp2, lres, bound;

    for (i = 0; i < 10; i++) {
        // tmp = accum_2(input[i]);

        ltmp  = 0;
        bound = input[i];
        for (li = 0; li < bound; li++)
            ltmp += li;

        ltmp2 = ltmp;
        lres  = 0;
        for (lj = 0; lj < ltmp2; lj++)
            lres += lj;
        // tmp = lres;

        if (lres < ref[i]) { // Due to if statement - dynamic II
            tmp2 = 0;
            for (k = 0; k < i; k++)
                tmp2 += i;

            tmp3 = tmp2;
            res2 = 0;
            for (j = 0; j < tmp3; j++)
                res2 += j;
            res += res2;
        }
    }
    return res;
}
