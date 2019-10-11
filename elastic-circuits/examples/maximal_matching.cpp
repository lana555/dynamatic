//========================================================================
// Maximal Matching
//========================================================================

//#include "maximal_matching.h"

int maximal_matching(int edges[], int vertices[], int num_edges) {

    int i   = 0;
    int out = 0;

    while (i < num_edges) {

        int j = i * 2;

        int u = edges[j];
        int v = edges[j + 1];

        if ((vertices[u] < 0) && (vertices[v] < 0)) {
            vertices[u] = v;
            vertices[v] = u;

            out = out + 1;
        }

        i = i + 1;
    }

    return out;
}
