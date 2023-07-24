int func(int x, int y, int w, int z) { return x * y * w * z; }

int test1(int a, int b, int c, int d, int f) {
    b++;
    f--;
    d += 4;
    for (int i = 0; i < 4; i++) {
        c = func(a, b, d, f);
    }
    return c;
}
