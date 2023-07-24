//========================================================================
// Sort
//========================================================================

// int min(int a, int b) {
//    if (a > b) return b;
//    else return a;
//}

// int max(int a, int b) {
//    if (a > b) return a;
//    else return b;
//}

void sort(int array[]) {
    int prev = 0;
    for (int i = 0; i < 100; ++i) {
        for (int j = 1; j < 100; ++j) {

            if (j == 1)
                prev = array[0];

            int curr     = array[j];
            array[j - 1] = (prev > curr ? curr : prev); // min( prev, curr );
            prev         = (prev > curr ? prev : curr); // max( prev, curr );

            if (j == 99)
                array[99] = prev;
        }
    }
}
