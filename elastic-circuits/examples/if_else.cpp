int dummy(int a, int b) {
    int k = a - b;
    if (a > b) {
        k = k * a;
        return k;
    } else if (b > a) {
        k = k * b;
        return k;
    } else
        return k;
}
