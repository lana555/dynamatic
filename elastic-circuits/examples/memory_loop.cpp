void memory(int x[1000], int y[1000]) {

    for (int i = 1; i < 1000; i++) {
        x[i] = x[i] * y[i] + x[0];
    }
}
