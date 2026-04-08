#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N 100000000
#define BINS 256

int main() {
    unsigned char* A = (unsigned char*)malloc(N * sizeof(unsigned char));
    int hist_naive[BINS] = {0};
    int hist_critical[BINS] = {0};
    int hist_reduction[BINS] = {0};
    
    for (int i = 0; i < N; i++) {
        A[i] = rand() % BINS;
    }
    
    double start, end;
    
    start = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        hist_naive[A[i]]++;
    }
    end = omp_get_wtime();
    printf("Naive version (race condition): %.4f seconds\n", end - start);
    
    start = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        #pragma omp critical
        {
            hist_critical[A[i]]++;
        }
    }
    end = omp_get_wtime();
    printf("Critical section version: %.4f seconds\n", end - start);
    
    start = omp_get_wtime();
    #pragma omp parallel for reduction(+:hist_reduction[:BINS])
    for (int i = 0; i < N; i++) {
        hist_reduction[A[i]]++;
    }
    end = omp_get_wtime();
    printf("Reduction version: %.4f seconds\n", end - start);
    
    int total = 0;
    for (int i = 0; i < BINS; i++) {
        total += hist_reduction[i];
    }
    printf("\nTotal count verified: %d\n", total);
    
    free(A);
    return 0;
}