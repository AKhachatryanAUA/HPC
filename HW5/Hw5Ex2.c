#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NUM_ORDERS 10000
#define NUM_THREADS 4

typedef struct {
    int order_id;
    double distance_km;
    char priority[10];
} DeliveryOrder;

int main() {
    DeliveryOrder* orders = (DeliveryOrder*)malloc(NUM_ORDERS * sizeof(DeliveryOrder));
    int thread_high_count[NUM_THREADS] = {0};
    double threshold;
    
    omp_set_num_threads(NUM_THREADS);
    
    for (int i = 0; i < NUM_ORDERS; i++) {
        orders[i].order_id = i + 1;
        orders[i].distance_km = (rand() % 5000) / 100.0;
    }
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            threshold = 20.0;
        }
        
        #pragma omp for
        for (int i = 0; i < NUM_ORDERS; i++) {
            if (orders[i].distance_km < threshold) {
                sprintf(orders[i].priority, "HIGH");
            } else {
                sprintf(orders[i].priority, "NORMAL");
            }
        }
        
        #pragma omp barrier
        
        #pragma omp single
        {
            printf("Priority assignment is finished.\n");
        }
        
        #pragma omp for
        for (int i = 0; i < NUM_ORDERS; i++) {
            if (orders[i].distance_km < threshold) {
                thread_high_count[omp_get_thread_num()]++;
            }
        }
        
        #pragma omp barrier
        
        #pragma omp single
        {
            int total_high = 0;
            for (int t = 0; t < NUM_THREADS; t++) {
                printf("Thread %d HIGH count: %d\n", t, thread_high_count[t]);
                total_high += thread_high_count[t];
            }
            printf("Total HIGH priority orders: %d\n", total_high);
        }
    }
    
    free(orders);
    return 0;
}