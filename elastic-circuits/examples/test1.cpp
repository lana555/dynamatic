void test1(int a[4], int n) {
    for (int i = 0; i < n; i++) {
        a[i] = a[i] + a[i - 1] + a[i - 1] + 4;
    }
    for (int i = 0; i < n; i++) {
        a[i] = a[i] + a[i - 1] + 5;
    }
}
