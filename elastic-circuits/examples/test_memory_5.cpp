void test1(int a[4], int n) {
    int x;
    for (int i = 0; i < n; i++) {
        a[i] = a[i - 1] + a[i - 2] + 5;
    }
}
