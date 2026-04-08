#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <float.h>

#define N 50000000

int main() {
    double* A = (double*)malloc(N * sizeof(double));
    double global_min = DBL_MAX;
    
    for (int i = 0; i < N; i++) {
        A[i] = (double)rand() / RAND_MAX * 1000.0;
    }
    
    double start = omp_get_wtime();
    
    #pragma omp parallel
    {
        double local_min = DBL_MAX;
        
        #pragma omp for
        for (int i = 1; i < N; i++) {
            double diff = A[i] - A[i-1];
            if (diff < 0) diff = -diff;
            if (diff < local_min) {
                local_min = diff;
            }
        }
        
        #pragma omp critical
        {
            if (local_min < global_min) {
                global_min = local_min;
            }
        }
    }
    
    double end = omp_get_wtime();
    
    printf("Global minimum distance: %.10f\n", global_min);
    printf("Time taken: %.4f seconds\n", end - start);
    
    free(A);
    return 0;
}