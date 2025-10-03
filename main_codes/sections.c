#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

static inline double work_kernel(long iters, double seed) {
    volatile double acc = 0.0;
    for (long i = 0; i < iters; ++i) {
        acc += sin(seed + 1e-9 * i) * cos(seed + 1e-9 * (i + 1));
    }
    return (double)acc;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <num_sections:2..4> <iters_per_section>\n", argv[0]);
        return 1;
    }
    int sections = atoi(argv[1]);
    if (sections < 2 || sections > 4) {
        fprintf(stderr, "num_sections must be 2..4\n");
        return 1;
    }
    long iters = atol(argv[2]);
    if (iters <= 0) {
        fprintf(stderr, "iters_per_section must be > 0\n");
        return 1;
    }

    int threads = omp_get_max_threads();
    double t0 = omp_get_wtime();
    double s1=0, s2=0, s3=0, s4=0;

    #pragma omp parallel sections
    {
        #pragma omp section
        { if (sections >= 1) s1 = work_kernel(iters, 1.0); }

        #pragma omp section
        { if (sections >= 2) s2 = work_kernel(iters, 2.0); }

        #pragma omp section
        { if (sections >= 3) s3 = work_kernel(iters, 3.0); }

        #pragma omp section
        { if (sections >= 4) s4 = work_kernel(iters, 4.0); }
    }

    double elapsed = omp_get_wtime() - t0;

    // CSV: Sections,Threads,Work,Time
    printf("%d,%d,%ld,%.6f\n", sections, threads, iters, elapsed);
    return 0;
}
