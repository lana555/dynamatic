void dummy(int n, int k) {
    int arr[n];
    for (int i = 1; i < n; i++) {
        for (int j = 1; j < n; j++)
            arr[i] = k + arr[i - 1];
    }
    // if(n < 10)
    //	arr[0] = n;

    for (int i = 1; i < n; i++) {
        arr[i] = k + arr[i - 1];
    }
    int arr2[n];
    for (int i = 1; i < n; i++) {
        for (int j = 1; j < n; j++)
            for (int m = 1; m < n; m++)
                arr2[i] = k + arr2[i - 1];
    }
}
