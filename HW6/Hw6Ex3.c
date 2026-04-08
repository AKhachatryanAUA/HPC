#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N 100000000

int main() {
    double* A = (double*)malloc(N * sizeof(double));
    
    for (int i = 0; i < N; i++) {
        A[i] = (double)rand() / RAND_MAX * 100.0;
    }
    
    double start, end;
    double max_val = 0.0;
    double threshold, sum = 0.0;
    
    start = omp_get_wtime();
    
    #pragma omp parallel for reduction(max:max_val)
    for (int i = 0; i < N; i++) {
        if (A[i] > max_val) {
            max_val = A[i];
        }
    }
    
    threshold = 0.8 * max_val;
    
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < N; i++) {
        if (A[i] > threshold) {
            sum += A[i];
        }
    }
    
    end = omp_get_wtime();
    
    printf("Maximum value: %.6f\n", max_val);
    printf("Threshold (0.8 * max): %.6f\n", threshold);
    printf("Sum of elements above threshold: %.6f\n", sum);
    printf("Time taken: %.4f seconds\n", end - start);
    
    free(A);
    return 0;
}