void accum(int a[10], int* y) {
    int x = 0;
    for (int i = 0; i < 10; i++) {
        x += a[i];
    }
    *y = x;
}
