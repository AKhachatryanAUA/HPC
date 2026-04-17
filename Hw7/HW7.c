#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define THRESHOLD 6

long long fib_omp(int n) {
    long long x, y;
    
    if (n <= 1) {
        return n;
    }
    
    if (n <= THRESHOLD) {
        return fib_omp(n - 1) + fib_omp(n - 2);
    }
    
    #pragma omp task shared(x)
    {
        x = fib_omp(n - 1);
    }
    
    #pragma omp task shared(y)
    {
        y = fib_omp(n - 2);
    }
    
    #pragma omp taskwait
    
    return x + y;
}

int main(int argc, char *argv[]) {
    int n;
    long long result;
    double start_time, end_time;
    
    if (argc != 2) {
        printf("Usage: %s <number>\n", argv[0]);
        return 1;
    }
    
    n = atoi(argv[1]);
    
    if (n < 0) {
        printf("Please enter a non-negative integer.\n");
        return 1;
    }
    
    start_time = omp_get_wtime();
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            result = fib_omp(n);
        }
    }
    
    end_time = omp_get_wtime();
    
    printf("Fibonacci(%d) = %lld\n", n, result);
    printf("Time taken: %f seconds\n", end_time - start_time);
    
    return 0;
}
