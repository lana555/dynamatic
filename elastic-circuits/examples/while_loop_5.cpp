//------------------------------------------------------------------------
// While loop
//------------------------------------------------------------------------

void while_loop(float a[], float b[], float c[]) {
    int i       = 0;
    float bound = 1000.0;
    float sum   = 0.0;

    while (sum < bound) {

        sum  = a[i] + b[i];
        c[i] = sum;
        i++;
    }
}
