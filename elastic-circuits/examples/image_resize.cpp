
void image_resize(int a[30][30], int c) {
    for (int i = 0; i < 30; i++) {
        for (int j = 0; j < 30; j++) {
            a[i][j] = c - a[i][j];
        }
    }
}
