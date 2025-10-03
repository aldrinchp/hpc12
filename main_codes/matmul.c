// src/matmul.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

// Utilidades de memoria alineada (mejor para SIMD/llc)
static double* aligned_alloc_d(size_t n) {
    void *p = NULL;
    if (posix_memalign(&p, 64, n * sizeof(double)) != 0) return NULL;
    return (double*)p;
}

static void fill_matrix(double *A, int n, double scale) {
    // determinístico y ligero
    for (int i=0;i<n;i++) for (int j=0;j<n;j++)
        A[i*(size_t)n + j] = scale * ((i*1315423911u + j*2654435761u) & 0xFF) / 255.0;
}

static double rel_error(const double *C, const double *Ref, int n) {
    double num=0.0, den=0.0;
    for (int i=0;i<n;i++) for (int j=0;j<n;j++) {
        double d = C[i*(size_t)n + j] - Ref[i*(size_t)n + j];
        num += d*d;
        den += Ref[i*(size_t)n + j]*Ref[i*(size_t)n + j];
    }
    return (den==0.0) ? sqrt(num) : sqrt(num/den);
}

// --- Baseline secuencial (para check) ---
// Orden i-k-j (mejor locality: reusa A[i,k] y recorre B[k,*] por filas)
static void matmul_seq(const double *A, const double *B, double *C, int n) {
    for (int i=0;i<n;i++) {
        for (int k=0;k<n;k++) {
            double r = A[i*(size_t)n + k];
            const double *Bk = &B[k*(size_t)n];
            double *Ci = &C[i*(size_t)n];
            for (int j=0;j<n;j++) {
                Ci[j] += r * Bk[j];
            }
        }
    }
}

// --- Variante 1: single (paraleliza i) ---
static void matmul_single(const double *A, const double *B, double *C, int n) {
    #pragma omp parallel for schedule(static)
    for (int i=0;i<n;i++) {
        for (int k=0;k<n;k++) {
            double r = A[i*(size_t)n + k];
            const double *Bk = &B[k*(size_t)n];
            double *Ci = &C[i*(size_t)n];
            for (int j=0;j<n;j++) {
                Ci[j] += r * Bk[j];
            }
        }
    }
}

// --- Variante 2: collapse(2) sobre (i,k) ---
static void matmul_collapse(const double *A, const double *B, double *C, int n) {
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i=0;i<n;i++) {
        for (int k=0;k<n;k++) {
            double r = A[i*(size_t)n + k];
            const double *Bk = &B[k*(size_t)n];
            double *Ci = &C[i*(size_t)n];
            for (int j=0;j<n;j++) {
                Ci[j] += r * Bk[j];
            }
        }
    }
}

// --- Variante 3: bloqueada (tiling) ---
static void matmul_blocked(const double *A, const double *B, double *C, int n, int BS) {
    #pragma omp parallel for collapse(2) schedule(static)
    for (int ii=0; ii<n; ii+=BS) {
        for (int kk=0; kk<n; kk+=BS) {
            for (int jj=0; jj<n; jj+=BS) {

                int iimax = (ii+BS<n)? ii+BS : n;
                int kkmax = (kk+BS<n)? kk+BS : n;
                int jjmax = (jj+BS<n)? jj+BS : n;

                for (int i=ii; i<iimax; ++i) {
                    for (int k=kk; k<kkmax; ++k) {
                        double r = A[i*(size_t)n + k];
                        const double *Bk = &B[k*(size_t)n];
                        double *Ci = &C[i*(size_t)n];
                        for (int j=jj; j<jjmax; ++j) {
                            Ci[j] += r * Bk[j];
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage:\n  %s <variant:single|collapse|blocked> <n> <BS> <repeats>\n", argv[0]);
        fprintf(stderr, "Note: BS ignored unless variant=blocked\n");
        return 1;
    }
    const char *variant = argv[1];
    int n   = atoi(argv[2]);
    int BS  = atoi(argv[3]);
    int R   = atoi(argv[4]);
    if (n<=0 || R<=0) { fprintf(stderr,"n>0, repeats>0\n"); return 1; }
    if (BS<=0) BS=64;

    size_t N = (size_t)n*(size_t)n;
    double *A = aligned_alloc_d(N);
    double *B = aligned_alloc_d(N);
    double *C = aligned_alloc_d(N);
    double *Ref = aligned_alloc_d(N);
    if(!A||!B||!C||!Ref){ fprintf(stderr,"alloc fail\n"); return 1; }

    fill_matrix(A, n, 1.0);
    fill_matrix(B, n, 1.0);
    memset(C,   0, N*sizeof(double));
    memset(Ref, 0, N*sizeof(double));

    // referencia (una vez)
    matmul_seq(A,B,Ref,n);

    // warmup ligero para estabilizar
    volatile double sink = 0.0; sink += A[0]+B[0];

    double best=1e99, acc=0.0;
    for (int r=0; r<R; ++r) {
        memset(C, 0, N*sizeof(double));
        double t0=omp_get_wtime();
        if      (strcmp(variant,"single")==0)   matmul_single(A,B,C,n);
        else if (strcmp(variant,"collapse")==0) matmul_collapse(A,B,C,n);
        else if (strcmp(variant,"blocked")==0)  matmul_blocked(A,B,C,n,BS);
        else { fprintf(stderr,"unknown variant %s\n", variant); return 1; }
        double t1=omp_get_wtime();
        double dt=t1-t0;
        if (dt<best) best=dt;
        acc += dt;
    }
    double time = acc/R;
    double gflops = (2.0 * (double)n * (double)n * (double)n) / time * 1e-9;

    double err = rel_error(C,Ref,n);

// hilos efectivos usados
int threads = 1;
#pragma omp parallel
{
    #pragma omp single
    threads = omp_get_num_threads();
}

// CSV: Variant,Threads,n,BS,Time,GFLOPs,RelError
printf("%s,%d,%d,%d,%.6f,%.6f,%.3e\n",
       variant, threads, n, BS, time, gflops, err);

    free(A); free(B); free(C); free(Ref);
    return 0;
}
