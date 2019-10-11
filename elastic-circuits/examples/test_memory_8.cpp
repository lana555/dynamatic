void test1(int a[4], int n) {
    int x;
    for (int i = 0; i < n; i++) {
        x = a[i - 1] + a[i - 2] + 5;
        if (x > 0) {
            a[i - 1] = a[i];
            a[i]     = 0;
        }
    }
}
