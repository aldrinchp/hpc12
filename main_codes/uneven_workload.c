#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <string.h>

double heavy_computation(int i) {
    double result = 0.0;
    int iterations = 1000 + (i % 1000); // workload depends on i
    for (int j = 0; j < iterations; j++) {
        result += sin(i * 0.001) * cos(j * 0.001);
    }
    return result;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s <schedule> <chunk> <threads>\n", argv[0]);
        return 1;
    }

    char *sched_str = argv[1];
    int chunk = atoi(argv[2]);
    int threads = atoi(argv[3]);

    omp_sched_t schedule;
    if (strcmp(sched_str, "static") == 0) schedule = omp_sched_static;
    else if (strcmp(sched_str, "dynamic") == 0) schedule = omp_sched_dynamic;
    else if (strcmp(sched_str, "guided") == 0) schedule = omp_sched_guided;
    else {
        printf("Unknown schedule: %s\n", sched_str);
        return 1;
    }

    omp_set_num_threads(threads);
    omp_set_schedule(schedule, chunk);

    int N = 10000; // total iterations
    double total = 0.0;

    double start = omp_get_wtime();

    #pragma omp parallel for reduction(+:total)
    for (int i = 0; i < N; i++) {
        total += heavy_computation(i);
    }

    double end = omp_get_wtime();

    printf("Schedule: %s, Chunk: %d, Threads: %d\n", sched_str, chunk, threads);
    printf("Total result = %f\n", total);
    printf("Execution time: %f seconds\n", end - start);

    return 0;
}