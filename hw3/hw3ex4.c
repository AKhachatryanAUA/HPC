#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_barrier_t barrier1;
pthread_barrier_t barrier2;
pthread_barrier_t barrier3;
int M = 4;

void* worker(void* arg) {
    int id = *(int*)arg;
    
    printf("Stage 1 completed by thread %d\n", id);
    pthread_barrier_wait(&barrier1);
    
    printf("Stage 2 completed by thread %d\n", id);
    pthread_barrier_wait(&barrier2);
    
    printf("Stage 3 completed by thread %d\n", id);
    pthread_barrier_wait(&barrier3);
    
    return NULL;
}

int main() {
    //TASK 4
    printf(".    Task 4");
    pthread_t threads[M];
    int id[M];
    
    pthread_barrier_init(&barrier1, NULL, M);
    pthread_barrier_init(&barrier2, NULL, M);
    pthread_barrier_init(&barrier3, NULL, M);
    
    for(int i = 0; i < M; i++) {
        id[i] = i;
        pthread_create(&threads[i], NULL, worker, &id[i]);
    }
    
    for(int i = 0; i < M; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_barrier_destroy(&barrier1);
    pthread_barrier_destroy(&barrier2);
    pthread_barrier_destroy(&barrier3);
    
    return 0;
}