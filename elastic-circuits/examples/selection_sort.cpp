void selection_sort(int A[], int n) {
    if (n <= 1)
        return;

    for (int i = 0; i < n - 1; i++) {
        int m = i;
        for (int j = i + 1; j < n; ++j) {
            if (A[j] > A[m])
                m = j;
        }
        int tmp = A[m];
        A[m]    = A[i];
        A[i]    = tmp;
    }
}
