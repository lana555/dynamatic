
/* Let INFINITY be the maximum value for an int */

typedef struct {
    int source;
    int dest;
    int weight;
} Edge;

#define EXT_DATASET
#ifndef EXT_DATASET
#define N_dest 10
#define N_dist 5
#else
#define N_dest 21
#define N_dist 15
#endif

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#define INFINITY INT_MAX

typedef int in_source;
typedef int in_dest;
typedef int in_weight;
typedef int in_distance;
typedef int in_edgecount;
typedef int in_nodecount;
typedef int in_src;

bool bellmanford(in_source source[N_dest], in_dest dest[N_dest], in_weight weight[N_dest],
                 in_distance distance[N_dist], in_edgecount edgecount, in_nodecount nodecount,
                 in_src src) {
    int i, j;
    bool succeeded = true;

    for (i = 0; i < nodecount; ++i)
        distance[i] = INFINITY;
    distance[src] = 0;

    for (i = 0; i < nodecount; ++i) {
        for (j = 0; j < edgecount; ++j) {
            if (distance[source[j]] != INFINITY) {
                int new_distance = distance[source[j]] + weight[j];
                if (new_distance < distance[dest[j]])
                    distance[dest[j]] = new_distance;
            }
        }
    }

    for (i = 0; i < edgecount; ++i) {
        if (distance[dest[i]] > distance[source[i]] + weight[i]) {
            succeeded = false;
            i         = edgecount;
        }
    }

    return succeeded;
}
