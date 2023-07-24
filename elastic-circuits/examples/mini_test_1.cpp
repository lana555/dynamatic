int test1(int a[100], int b[100], int n) {
    for (int i = 0; i < 100; i++) {
        a[i]     = i * 5;
        b[n + 1] = n * 6;
        n        = n + 1;
    }
    return n;
}
