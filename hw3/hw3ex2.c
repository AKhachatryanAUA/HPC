#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_barrier_t barrier;
int N = 4;

void* player(void* arg) {
    int id = *(int*)arg;
    int ready_time = rand() % 3 + 1;
    sleep(ready_time);
    printf("Player %d is ready\n", id + 1);
    pthread_barrier_wait(&barrier);
    return NULL;
}

int main() {
    //TASK 2
    printf("     Task 2 \n");
    srand(time(NULL));
    pthread_t threads[N];
    int id[N];
    pthread_barrier_init(&barrier, NULL, N + 1);
    for(int i = 0; i < N; i++) {
        id[i] = i;
        pthread_create(&threads[i], NULL, player, &id[i]);
    }
    pthread_barrier_wait(&barrier);
    for(int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_barrier_destroy(&barrier);
    return 0;
}