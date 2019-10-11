// prefixsum: cumulative sum of a sequence of numbers, computed by the recurrence equation
// Stephen HLS book

void prefixsum(int in[1000], int out[1000]) {
    out[0] = in[0];
    for (int i = 1; i < 1000; i++) {
        out[i] = out[i - 1] + in[i];
    }
}