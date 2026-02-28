#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_barrier_t barrier;
pthread_mutex_t mutex;
int rounds = 5;
int players = 4;
int rolls[4];
int round_winner;

void* player(void* arg) {
    int id = *(int*)arg;
    
    for(int i = 0; i < rounds; i++) {
        rolls[id] = rand() % 6 + 1;
        pthread_barrier_wait(&barrier);
        if(id == 0) {
            int max = 0;
            round_winner = 0;
            
            for(int j = 0; j < players; j++) {
                if(rolls[j] > max) {
                    max = rolls[j];
                    round_winner = j;
                }
            }
            pthread_mutex_lock(&mutex);
            printf("Round winner is: %d \n", round_winner);
            pthread_mutex_unlock(&mutex);
        }
        pthread_barrier_wait(&barrier);
    }
    return NULL;
}

int main() {
    printf(".    Task 1\n");
    srand(time(NULL));
    
    pthread_t threads[players];
    int id[players];
    
    pthread_barrier_init(&barrier, NULL, players);
    pthread_mutex_init(&mutex, NULL);
    
    for(int i = 0; i < players; i++) {
        id[i] = i;
        pthread_create(&threads[i], NULL, player, &id[i]);
    }
    
    for(int i = 0; i < players; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    return 0;
}
