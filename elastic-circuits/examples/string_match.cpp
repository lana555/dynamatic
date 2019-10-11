// Finds an occurrence of x in y
// n is the length of x, m is the length of y
int substring(char x[], char y[], int n, int m) {
    for (int i = 0; i <= m - n; ++i) {
        int j = 0;
        while (j < n and x[j] == y[i + j])
            ++j;
        if (j == n)
            return i;
    }
    return -1;
}
