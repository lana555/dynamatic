void insertion_sort(int A[], int n) {
    if (n <= 1)
        return;

    for (int i = 1; i < n; ++i) {
        int x = A[i];
        int j = i;
        while (j > 0 and A[j - 1] > x) {
            A[j] = A[j - 1];
            --j;
        }
        A[j] = x;
    }
}
