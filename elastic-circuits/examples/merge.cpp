void merge(int A[], int n, int B[], int m, int C[]) {
    int iA = 0, iB = 0, iC = 0;

    while (iA < n and iB < m) {
        if (A[iA] < B[iB])
            C[iC++] = A[iA++];
        else
            C[iC++] = B[iB++];
    }

    while (iA < n)
        C[iC++] = A[iA++];
    while (iB < m)
        C[iC++] = B[iB++];
}
