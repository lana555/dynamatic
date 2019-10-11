void nested_loop(int n, int k) {
    int arr[n];
    for (int i = 1; i < n; i++) {
        for (int j = 1; j < n; j++)
            arr[i] = k + arr[i - 1];
    }
}
