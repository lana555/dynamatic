void test1(int a[40], int n) {
    int x;
    for (int i = 2; i < n; i++) {
        a[i] = a[i - 1] + a[i - 2] + 5;
        if (a[i] > 0)
            a[i] = 0;
    }
}

int main() {
    int a[40];
    for (int i = 0; i < 40; i++)
        a[i] = i;
    test1(a, 40);
    return 0;
}