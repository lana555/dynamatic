void sum_tree(int a1[1000], int a2[1000], int a3[1000], int a4[1000], int a5[1000], int a6[1000],
              int a7[1000], int a8[1000], int a9[1000], int a10[1000], int a11[1000], int a12[1000],
              int a13[1000], int a14[1000], int a15[1000], int a16[1000], int out[1000]) {

    for (int i = 0; i < 1000; i++) {
        int o1  = a1[i] + a2[i];
        int o2  = a3[i] + a4[i];
        int o3  = a5[i] + a6[i];
        int o4  = a7[i] + a8[i];
        int o5  = a9[i] + a10[i];
        int o6  = a11[i] + a12[i];
        int o7  = a13[i] + a14[i];
        int o8  = a15[i] + a16[i];
        int o9  = o1 + o2;
        int o10 = o3 + o4;
        int o11 = o5 + o6;
        int o12 = o7 + o8;
        int o13 = o9 + o10;
        int o14 = o11 + o12;

        out[i] = o13 + o14;
    }
}