int func(int x) { return x * x; }

void test1(int n) {
    int a[n];
    for (int i = 0; i < n; i++) {
        a[i] = func(a[i]);
    }
}
