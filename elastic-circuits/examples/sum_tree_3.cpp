void sum_tree(int a1[1000], int a2[1000], int a3[1000], int a4[1000], int a5[1000], int a6[1000],
              int a7[1000], int a8[1000], int out[1000]) {

    for (int i = 0; i < 1000; i++) {
        int o1 = a1[i] + a2[i];
        int o2 = a3[i] + a4[i];
        int o3 = a5[i] + a6[i];
        int o4 = a7[i] + a8[i];
        int o6 = o1 + o2;
        int o7 = o3 + o4;
        out[i] = o6 + o7;
    }
}