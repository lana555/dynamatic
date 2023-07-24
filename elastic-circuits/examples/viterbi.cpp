typedef int tok_t;
typedef float prob_t;
typedef int state_t;
typedef int step_t;

int viterbi(tok_t obs[20], prob_t init[32], prob_t transition[1024], prob_t emission[1024],
            state_t path[20]) {
    prob_t llike[20][32];
    step_t t;
    state_t prev, curr;
    prob_t min_p, p;
    state_t min_s, s;
    // All probabilities are in -log space. (i.e.: P(x) => -log(P(x)) )

    // Initialize with first observation and initial probabilities
    for (s = 0; s < 32; s++) {
        int ind     = s * 32 + obs[0];
        llike[0][s] = init[s] + emission[ind];
    }

    // Iteratively compute the probabilities over time
    for (t = 1; t < 20; t++) {
        for (curr = 0; curr < 32; curr++) {
            // Compute likelihood HMM is in current state and where it came from.
            prev = 0;
            min_p =
                llike[t - 1][prev] + transition[prev * 32 + curr] + emission[curr * 32 + obs[t]];

            for (prev = 1; prev < 32; prev++) {
                p = llike[t - 1][prev] + transition[prev * 32 + curr] +
                    emission[curr * 32 + obs[t]];
                if (p < min_p) {
                    min_p = p;
                }
            }
            llike[t][curr] = min_p;
        }
    }

    // Identify end state
    min_s = 0;
    min_p = llike[19][min_s];
    for (s = 1; s < 32; s++) {
        p = llike[19][s];
        if (p < min_p) {
            min_p = p;
            min_s = s;
        }
    }
    path[19] = min_s;

    // Backtrack to recover full path
    for (t = 18; t >= 0; t--) {
        min_s = 0;
        min_p = llike[t][min_s] + transition[min_s * 32 + path[t + 1]];
        for (s = 1; s < 32; s++) {
            p = llike[t][s] + transition[s * 32 + path[t + 1]];
            if (p < min_p) {
                min_p = p;
                min_s = s;
            }
        }
        path[t] = min_s;
    }

    return 0;
}
