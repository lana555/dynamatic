void sum_tree(int a1[1000], int a2[1000], int a3[1000], int a4[1000], int out[1000]) {

    for (int i = 0; i < 1000; i++) {
        int o1 = a1[i] + a2[i];
        int o2 = a3[i] + a4[i];
        out[i] = o1 + o2;
    }
}