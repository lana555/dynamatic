
void threshold(int red[1000], int green[1000], int blue[1000], int th) {
    for (int i = 0; i < 1000; i++) {
        int sum = red[i] + green[i] + blue[i];

        if (sum <= th) {
            red[i]   = 0;
            green[i] = 0;
            blue[i]  = 0;
        }
    }
}
