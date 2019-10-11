int sumi3(int a[1000]) {
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        int x = a[i];
        sum += x * x * x;
    }
    return sum;
}