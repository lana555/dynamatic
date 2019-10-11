void dummy(int n, int k) {
    int arr[n];
    for (int i = 1; i < n; i++) {
        arr[i] = k + arr[i - 1];
    }
}

int main() {
    dummy(5, 4);
    return 0;
}