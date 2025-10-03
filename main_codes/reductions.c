// src/reductions.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>  // strcmp
#include <math.h>    // fabs
#include <omp.h>

static void fill_array(double *A, long N) {
    // determinístico para reproducibilidad
    for (long i = 0; i < N; ++i) A[i] = (double)(i % 1000) * 0.001;
}

static double seq_sum(const double *A, long N) {
    double s = 0.0;
    for (long i = 0; i < N; ++i) s += A[i];
    return s;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <mode:critical|atomic|reduction> <N> <repeats>\n", argv[0]);
        return 1;
    }
    const char *mode = argv[1];
    long N = atol(argv[2]);
    int repeats = atoi(argv[3]);
    if (N <= 0 || repeats <= 0) { fprintf(stderr, "N>0, repeats>0\n"); return 1; }

    double *A = (double*) malloc((size_t)N * sizeof(double));
    if (!A) { perror("malloc"); return 1; }
    fill_array(A, N);
    const double ref = seq_sum(A, N);  // referencia secuencial

    // Calentamiento ligero
    volatile double sink = 0.0;
    for (int w = 0; w < 1000; ++w) sink += A[w % N];

    // --- medición promediando 'repeats' veces ---
    double best = 1e99, acc_time = 0.0;
    double last_sum = 0.0;

    for (int r = 0; r < repeats; ++r) {
        double sum = 0.0;
        double t0 = omp_get_wtime();

        if (strcmp(mode, "critical") == 0) {
            #pragma omp parallel for
            for (long i = 0; i < N; ++i) {
                #pragma omp critical
                sum += A[i];
            }
        } else if (strcmp(mode, "atomic") == 0) {
            #pragma omp parallel for
            for (long i = 0; i < N; ++i) {
                #pragma omp atomic
                sum += A[i];
            }
        } else if (strcmp(mode, "reduction") == 0) {
            #pragma omp parallel for reduction(+:sum)
            for (long i = 0; i < N; ++i) {
                sum += A[i];
            }
        } else {
            fprintf(stderr, "Unknown mode: %s\n", mode);
            free(A);
            return 1;
        }

        double t1 = omp_get_wtime();
        double dt = t1 - t0;
        if (dt < best) best = dt;
        acc_time += dt;
        last_sum = sum; // guarda el último para reportar
    }

    // Número de hilos efectivos (sin usar atomics inválidos)
    int threads = 0;
    #pragma omp parallel
    {
        #pragma omp single
        threads = omp_get_num_threads();
    }

    // correctitud (tolerancia relativa por redondeo)
    double rel = (ref == 0.0) ? 0.0 : fabs((last_sum - ref) / ref);
    int correct = (rel < 1e-12) ? 1 : 0;

    // Reporte CSV (tiempo promedio; cambia a 'best' si prefieres el mejor)
    double time = acc_time / repeats;
    printf("%s,%d,%ld,%.6f,%.17g,%d\n", mode, threads, N, time, last_sum, correct);

    free(A);
    return 0;
}
