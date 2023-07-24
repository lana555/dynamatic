

void mat_mul(int a[100], int b[100], int c[2]){
    int temp = 0;
    for(int i = 0; i < 100; ++i){
        temp += a[i] * b[i];
    }
    c[0] = temp;
    c[1] = temp;
}
