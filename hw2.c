#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <math.h>
#include <sched.h>
#define ARRAY_SIZE 50000000
#define CHUNK_SIZE 10000000
#define ITERATIONS 1000000000

void* thread_func(void* arg) {
    int thread_id = *(int*)arg;
    long long sum = 0;
    
    for(long long i = 0; i < ITERATIONS; i++) {
        sum += i;
        if(i % 100000000 == 0) {
            int cpu = sched_getcpu();
            printf("Thread %d on CPU %d\n", thread_id, cpu);
        }
    }
    
    return NULL;
}


void* printName(void* helper_id){
    printf("thread %p is running \n", pthread_self());
    return 0;
}
typedef struct{
    int* array;
    int start;
    int finish;
    int partialSum;
    int max;
}arrayArgs;


typedef struct{
    int start;
    int finish;
    int primeCount;
}primeArgs;

void* arrayCounter(void* arg){
    arrayArgs* args = (arrayArgs*) arg;
    int sum=0;
    for(int i=args -> start; i<args -> finish; i++){
        sum+= args -> array[i];
    }
    args ->partialSum=sum;
    return NULL;
}
bool isPrime(int num){
    if( num <= 1) return false;
    if(num % 2 == 0) return false;
    for(int i = 3; i < sqrt(num); i += 2){
    
        if(num % i == 0){
                return false;
        }
    }
    return true;
}

void* countPrimes(void* arg){
    primeArgs* args = (primeArgs*) arg;
    int count=0;
    for(int i=args -> start; i< args -> finish; i++){
        if(isPrime(i)){
            count++;
        } 
    }
    args ->primeCount=count;
    return NULL;
}

void* arrayMax(void* arg){
    arrayArgs* args=(arrayArgs*) arg;
    int max=0;
    for(int i=args -> start; i<args -> finish; i++){
        if(args -> array[i] > max){
            max=args -> array[i];
        }
    }
    args -> max = max;
    return NULL;
}



int main(){
    int* array= malloc(ARRAY_SIZE*sizeof(int));
    if(array ==NULL){
        perror("space not allocated");
        exit(EXIT_FAILURE);
    }
    srand(time(NULL));
    int oneThreadSum=0;
    for(int i=0; i<ARRAY_SIZE; i++ ){
        array[i]=rand();
        oneThreadSum+=array[i];
    }
    //TASK 1
    printf(".    Task 1 \n");
    pthread_t thread1,thread2, thread3;
    pthread_create(&thread1,NULL,printName,NULL);
    pthread_create(&thread2,NULL,printName,NULL);
    pthread_create(&thread3,NULL,printName,NULL);

    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);
    pthread_join(thread3,NULL);

    //TASK 2
    printf(".    Task 2 \n");
    
    //1.
    printf("One thread sum is: %d \n",oneThreadSum);
    
    //2.
    pthread_t threads[5];
    arrayArgs args[5];
      for(int i = 0; i < 5; i++) {
        args[i].array = array;
        args[i].start = i * CHUNK_SIZE;
        args[i].finish = (i == 4) ? ARRAY_SIZE : (i + 1) * CHUNK_SIZE;
        args[i].partialSum = 0; 
        
        pthread_create(&threads[i], NULL, arrayCounter, &args[i]);
    }
    int parralelSum=0;
    for(int i=0; i<5; i++){
        pthread_join(threads[i],NULL);
        parralelSum+= args[i].partialSum;
    }
    printf("Parralel sum is: %d \n",parralelSum);


    //TASK 3
    printf(".    Task 3 \n");

    // 1.
    int max=0;
    for(int i=0; i< ARRAY_SIZE; i++){
        if(array[i]>max){
            max=array[i];
        }
    }
    printf("Max found sequentially is: %d \n", max);


    // 2.
    pthread_t threadsForMax[4];
      for(int i = 0; i < 4; i++) {
        args[i].array = array;
        args[i].start = i * CHUNK_SIZE;
        args[i].finish =(i + 1) * CHUNK_SIZE;
        args[i].partialSum = 0; 
        
        pthread_create(&threadsForMax[i], NULL, arrayMax, &args[i]);
    }
    
    for(int i =0; i<4; i++){
        pthread_join(threadsForMax[i],NULL);
        if(args[i].max> max){
            max=args[i].max;
        }
    }
    printf("Max counted with 4 threads is: %d \n",max);

    //TASK 4
    printf(".    Task 4 \n");
    // 1.
    int OneThreadPrimeCount = 0;
    for(int i=0; i<20000000; i++){
        if(isPrime(i)){
            OneThreadPrimeCount++;
        }
    }
    printf("number of prime numbers counted with one thread: %d \n", OneThreadPrimeCount);

    // 2.
    pthread_t threadsforPrimes[4];
    primeArgs primeArgs[4];
    int parralelPrimeCount=0;
    for(int i = 0; i< 4; i++){
        primeArgs[i].start=i*5000000;
        primeArgs[i].finish=(i+1)*5000000;
        primeArgs[i].primeCount=0;
        pthread_create(&threadsforPrimes[i],NULL, countPrimes, &primeArgs[i]);
    }
    for(int i=0; i<4; i++){
        pthread_join(threadsforPrimes[i],NULL);
        parralelPrimeCount += primeArgs[i].primeCount;
    }
    printf("number of prime numbers counted in parralel: %d \n", parralelPrimeCount);

    //TASK 5
    // I am working on mac function sched_getcpu() is not working here
    printf(".    Task 5");
    pthread_t threadsForId[5];
    int thread_ids[5];
    
    for(int i = 0; i < 5; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    
    for(int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

}