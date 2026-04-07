#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define NUM_LOGS 20000
#define NUM_THREADS 4

typedef struct {
    int request_id;
    int user_id;
    int response_time_ms;
} LogEntry;

int main() {
    LogEntry* logs = (LogEntry*)malloc(NUM_LOGS * sizeof(LogEntry));
    int fast_count = 0, medium_count = 0, slow_count = 0;
    
    omp_set_num_threads(NUM_THREADS);
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            srand(time(NULL));
            for (int i = 0; i < NUM_LOGS; i++) {
                logs[i].request_id = i + 1;
                logs[i].user_id = (rand() % 1000) + 1;
                logs[i].response_time_ms = rand() % 1000;
            }
        }
        
        #pragma omp barrier
        
        #pragma omp for
        for (int i = 0; i < NUM_LOGS; i++) {
            int rt = logs[i].response_time_ms;
            if (rt < 100) {
                #pragma omp atomic
                fast_count++;
            } else if (rt <= 300) {
                #pragma omp atomic
                medium_count++;
            } else {
                #pragma omp atomic
                slow_count++;
            }
        }
        
        #pragma omp barrier
        
        #pragma omp single
        {
            printf("FAST: %d\n", fast_count);
            printf("MEDIUM: %d\n", medium_count);
            printf("SLOW: %d\n", slow_count);
        }
    }
    
    free(logs);
    return 0;
}