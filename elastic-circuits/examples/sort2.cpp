//========================================================================
// Sort
//========================================================================

int min(int a, int b) {
    if (a > b)
        return b;
    else
        return a;
}

int max(int a, int b) {
    if (a > b)
        return a;
    else
        return b;
}

void sort(int array[], int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 1; j < n; ++j) {
            int prev;
            if (j == 1)
                prev = array[0];

            int curr     = array[j];
            array[j - 1] = min(prev, curr);
            prev         = max(prev, curr);

            if (j == n - 1)
                array[n - 1] = prev;
        }
    }
}
