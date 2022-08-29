#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_FUNC_COUNT 100

struct func_frequencies {
    int initialized;
    int BB_count;
    int **frequencies;
} ;

struct func_frequencies profiler[MAX_FUNC_COUNT];

void init(int func_ID, int BB_count){
    if (profiler[func_ID].initialized == 1) return;
    profiler[func_ID].initialized = 1;
    profiler[func_ID].BB_count = BB_count;
    profiler[func_ID].frequencies = malloc((BB_count + 1) * sizeof(int *));
    for (int i = 1; i <= BB_count; i++) {
        profiler[func_ID].frequencies[i] = malloc((BB_count + 1) * sizeof(int));
    }
}

void uncond(int func_ID, int src, int dst){
    profiler[func_ID].frequencies[src][dst]++;
}

void cond(int func_ID, int condition, int src, int dst_true, int dst_false){
    if (condition) profiler[func_ID].frequencies[src][dst_true]++;
    else profiler[func_ID].frequencies[src][dst_false]++;
}

void report(){
    for (int func_ID = 0; func_ID < MAX_FUNC_COUNT; func_ID++) {
        if (profiler[func_ID].initialized != 1) continue;

        char file_name[20];
        sprintf(file_name, "%d_freq.txt", func_ID);
        FILE *fp = fopen(file_name, "w");

        int size = profiler[func_ID].BB_count;
        for (int i = 1; i <= size; i++) {
            for (int j = 1; j <= size; j++) {
                if (profiler[func_ID].frequencies[i][j] > 0) {
                    fprintf(fp, "%d %d %d\n", i, j, profiler[func_ID].frequencies[i][j]);
                }
            }
        }
        fclose(fp);
    }
}