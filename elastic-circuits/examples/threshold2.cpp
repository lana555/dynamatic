
void threshold(int red[1000], int green[1000], int blue[1000], int th) {
    for (int i = 0; i < 1000; i++) {
    	int x1 = red[i];
        int sum = x1 + green[i] + blue[i];

        if (sum <= th) {
            red[i]   = x1*0;
            green[i] = 0;
            blue[i]  = 0;
        }
    }
}
