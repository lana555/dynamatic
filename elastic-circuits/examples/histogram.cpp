//------------------------------------------------------------------------
// Histogram
//------------------------------------------------------------------------

void histogram(int feature[1000], float weight[1000], float hist[1000], int n) {
    for (int i = 0; i < n; ++i) {
        int m    = feature[i];
        float wt = weight[i];
        float x  = hist[m];
        hist[m]  = x + wt;
    }
}
