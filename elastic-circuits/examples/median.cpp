
void median(int a[1000], int weight[1000]) {
    for (int i = 0; i < 1000; i++) {
        a[i] = a[i - 1] * weight[i - 1] + a[i] * weight[i] + a[i + 1] * weight[i + 1];
    }
}
