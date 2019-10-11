
void video_filter_rescale(int pixel_red[30][30], int pixel_blue[30][30], int pixel_green[30][30],
                          int offset, int scale) {

    for (int row = 0; row < 30; row++) {

        for (int col = 0; col < 30; col++) {

            pixel_red[row][col]   = ((pixel_red[row][col] - offset) * scale) >> 4;
            pixel_blue[row][col]  = ((pixel_blue[row][col] - offset) * scale) >> 4;
            pixel_green[row][col] = ((pixel_green[row][col] - offset) * scale) >> 4;
        }
    }
}