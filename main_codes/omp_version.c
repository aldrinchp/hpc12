#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>   // Necesario para OpenMP

void vector_add(const float *a, const float *b, float *result, size_t n) {
    #pragma omp parallel for
    for (size_t i = 0; i < n; ++i) {
        result[i] = a[i] + b[i];
    }
}

void matrix_multiply(const float *A, const float *B, float *C, size_t rowsA, size_t colsA, size_t colsB) {
    float a_ik;
    #pragma omp parallel for collapse(2)   // Colapsamos i y j para más paralelismo
    for (size_t i = 0; i < rowsA; ++i) {
        for (size_t k = 0; k < colsA; ++k) {
            a_ik = A[i * colsA + k];
            for (size_t j = 0; j < colsB; ++j) {
                C[i * colsB + j] += a_ik * B[k * colsB + j];
            }
        }
    }
}

float* create_matrix(size_t rows, size_t cols) {
    float *matrix = (float*)malloc(rows * cols * sizeof(float));
    if (!matrix) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < rows * cols; ++i) {
        matrix[i] = (float)rand() / RAND_MAX; // Valor aleatorio entre 0 y 1
    }
    return matrix;
}

int main(int argc, char **argv) {
    
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <n>\n", argv[0]);
        return EXIT_FAILURE;
    }
    size_t n = (size_t)atoi(argv[1]);
    n = sqrt(n);
    float *a = create_matrix(1,n*n);
    float *b = create_matrix(1,n*n);
    float *c = malloc(sizeof(float)*n*n);

    float *A = create_matrix(n, n);
    float *B = create_matrix(n, n);
    float *C = malloc(sizeof(float)*n*n);

    double start, end;

    // Medir tiempo de suma de vectores
    start = omp_get_wtime();
    vector_add(a, b, c, n*n);
    end = omp_get_wtime();
    printf("Tiempo de suma de vectores: %f segundos\n", end - start);

    // Medir tiempo de multiplicación de matrices
    start = omp_get_wtime();
    matrix_multiply(A, B, C, n, n, n);
    end = omp_get_wtime();
    printf("Tiempo de multiplicación de matrices: %f segundos\n", end - start);

    free(a);
    free(b);
    free(c);
    free(A);
    free(B);
    free(C);

    return 0;
}
