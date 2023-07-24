
#define PATTERN_SIZE 4
#define STRING_SIZE 1000

int kmp(char pattern[PATTERN_SIZE], char input[STRING_SIZE], int kmpNext[PATTERN_SIZE]) {
    int i, q;
    int n_matches = 0;

    int k;
    k          = 0;
    kmpNext[0] = 0;

c1:
    for (q = 1; q < PATTERN_SIZE; q++) {
        char tmp = pattern[q];
    c2:
        while (k > 0 && pattern[k] != tmp) {
            k = kmpNext[q];
        }
        if (pattern[k] == tmp) {
            k++;
        }
        kmpNext[q] = k;
    }

    q = 0;
k1:
    for (i = 0; i < STRING_SIZE; i++) {
        char tmp = input[i];
    k2:
        while (q > 0 && pattern[q] != tmp) {
            q = kmpNext[q];
        }
        if (pattern[q] == tmp) {
            q++;
        }
        if (q >= PATTERN_SIZE) {
            n_matches++;
            q = kmpNext[q - 1];
        }
    }
    return n_matches;
}