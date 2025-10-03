#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main() {
    int N = 10000000; // tamaño del arreglo
    double *A = malloc(sizeof(double) * N);
    for (int i = 0; i < N; i++) A[i] = 1.0; // suma esperada = N

    double sum;
    double start, end;

    // ==================== Correct: reduction ====================
    sum = 0.0;
    start = omp_get_wtime();
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < N; i++) {
        sum += A[i];
    }
    end = omp_get_wtime();
    printf("Reduction: sum = %.1f, time = %f s\n", sum, end - start);

    // ==================== Incorrect: shared =====================
    sum = 0.0;
    start = omp_get_wtime();
    #pragma omp parallel for shared(sum)
    for (int i = 0; i < N; i++) {
        sum += A[i]; // race condition!
    }
    end = omp_get_wtime();
    printf("Shared (incorrect): sum = %.1f, time = %f s\n", sum, end - start);

    // ==================== Incorrect: private ====================
    sum = 0.0;
    start = omp_get_wtime();
    #pragma omp parallel for private(sum)
    for (int i = 0; i < N; i++) {
        sum += A[i]; // each thread has its own sum
    }
    end = omp_get_wtime();
    printf("Private (incorrect): sum = %.1f, time = %f s\n", sum, end - start);

    free(A);
    return 0;
}
