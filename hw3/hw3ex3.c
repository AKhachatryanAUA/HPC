#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_barrier_t barrier;
pthread_mutex_t mutex;
int S = 4;
int temperatures[4];
float sum = 0;

void* sensor(void* arg) {
    int id = *(int*)arg;
    temperatures[id] = rand() % 30 + 10;
    printf("Sensor %d collected: %d °C\n", id + 1, temperatures[id]);
    pthread_barrier_wait(&barrier);
    pthread_mutex_lock(&mutex);
    sum = sum + temperatures[id];
    pthread_mutex_unlock(&mutex);
    pthread_barrier_wait(&barrier);
    return NULL;
}

int main() {
    //TASK 2
    printf("     Task 3 \n");
    srand(time(NULL));
    pthread_t threads[S];
    int id[S];
    
    pthread_barrier_init(&barrier, NULL, S);
    pthread_mutex_init(&mutex, NULL);
    
    for(int i = 0; i < S; i++) {
        id[i] = i;
        pthread_create(&threads[i], NULL, sensor, &id[i]);
    }
    
    for(int i = 0; i < S; i++) {
        pthread_join(threads[i], NULL);
    }
    
    float average = sum / S;
    printf("Average temperature: %.2f°C\n", average);
    
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    return 0;
}