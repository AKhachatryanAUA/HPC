#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

// Check architecture and include appropriate headers
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
    #define USING_NEON 1
#elif defined(__AVX2__) || defined(__AVX__)
    #include <immintrin.h>
    #define USING_AVX 1
#else
    #include <emmintrin.h>  // SSE2 fallback
    #define USING_SSE 1
#endif

#define BUFFER_SIZE (256 * 1024 * 1024)
#define NUM_THREADS 4

typedef struct {
    char *buffer;
    size_t start;
    size_t end;
    unsigned long long counts[4];
} ThreadArgs;

void generate_dna(char *buffer, size_t size) {
    const char nucleotides[] = {'A', 'C', 'G', 'T'};
    for (size_t i = 0; i < size; i++) {
        buffer[i] = nucleotides[rand() % 4];
    }
}

void count_scalar(const char *buffer, size_t size, unsigned long long counts[4]) {
    counts[0] = counts[1] = counts[2] = counts[3] = 0;
    for (size_t i = 0; i < size; i++) {
        char c = buffer[i];
        if (c == 'A') counts[0]++;
        else if (c == 'C') counts[1]++;
        else if (c == 'G') counts[2]++;
        else if (c == 'T') counts[3]++;
    }
}

#if defined(USING_NEON)
// NEON version for Apple Silicon
void count_simd(const char *buffer, size_t size, unsigned long long counts[4]) {
    counts[0] = counts[1] = counts[2] = counts[3] = 0;
    
    uint8x16_t vecA = vdupq_n_u8('A');
    uint8x16_t vecC = vdupq_n_u8('C');
    uint8x16_t vecG = vdupq_n_u8('G');
    uint8x16_t vecT = vdupq_n_u8('T');
    
    size_t i = 0;
    unsigned int local_counts[4] = {0};
    
    for (; i + 16 <= size; i += 16) {
        uint8x16_t chunk = vld1q_u8((const uint8_t*)(buffer + i));
        
        uint8x16_t cmpA = vceqq_u8(chunk, vecA);
        uint8x16_t cmpC = vceqq_u8(chunk, vecC);
        uint8x16_t cmpG = vceqq_u8(chunk, vecG);
        uint8x16_t cmpT = vceqq_u8(chunk, vecT);
        
        // Convert boolean to 0/1 and sum
        uint8x16_t bitsA = vandq_u8(cmpA, vdupq_n_u8(1));
        uint8x16_t bitsC = vandq_u8(cmpC, vdupq_n_u8(1));
        uint8x16_t bitsG = vandq_u8(cmpG, vdupq_n_u8(1));
        uint8x16_t bitsT = vandq_u8(cmpT, vdupq_n_u8(1));
        
        local_counts[0] += vaddvq_u8(bitsA);
        local_counts[1] += vaddvq_u8(bitsC);
        local_counts[2] += vaddvq_u8(bitsG);
        local_counts[3] += vaddvq_u8(bitsT);
    }
    
    counts[0] = local_counts[0];
    counts[1] = local_counts[1];
    counts[2] = local_counts[2];
    counts[3] = local_counts[3];
    
    // Handle remaining bytes
    for (; i < size; i++) {
        char c = buffer[i];
        if (c == 'A') counts[0]++;
        else if (c == 'C') counts[1]++;
        else if (c == 'G') counts[2]++;
        else if (c == 'T') counts[3]++;
    }
}

#elif defined(USING_AVX)
// AVX2 version for Intel Macs
void count_simd(const char *buffer, size_t size, unsigned long long counts[4]) {
    counts[0] = counts[1] = counts[2] = counts[3] = 0;
    
    __m256i vecA = _mm256_set1_epi8('A');
    __m256i vecC = _mm256_set1_epi8('C');
    __m256i vecG = _mm256_set1_epi8('G');
    __m256i vecT = _mm256_set1_epi8('T');
    
    size_t i = 0;
    for (; i + 32 <= size; i += 32) {
        __m256i chunk = _mm256_loadu_si256((const __m256i*)(buffer + i));
        
        __m256i cmpA = _mm256_cmpeq_epi8(chunk, vecA);
        __m256i cmpC = _mm256_cmpeq_epi8(chunk, vecC);
        __m256i cmpG = _mm256_cmpeq_epi8(chunk, vecG);
        __m256i cmpT = _mm256_cmpeq_epi8(chunk, vecT);
        
        int maskA = _mm256_movemask_epi8(cmpA);
        int maskC = _mm256_movemask_epi8(cmpC);
        int maskG = _mm256_movemask_epi8(cmpG);
        int maskT = _mm256_movemask_epi8(cmpT);
        
        counts[0] += __builtin_popcount(maskA);
        counts[1] += __builtin_popcount(maskC);
        counts[2] += __builtin_popcount(maskG);
        counts[3] += __builtin_popcount(maskT);
    }
    
    for (; i < size; i++) {
        char c = buffer[i];
        if (c == 'A') counts[0]++;
        else if (c == 'C') counts[1]++;
        else if (c == 'G') counts[2]++;
        else if (c == 'T') counts[3]++;
    }
}

#else
// SSE2 fallback for older systems
void count_simd(const char *buffer, size_t size, unsigned long long counts[4]) {
    counts[0] = counts[1] = counts[2] = counts[3] = 0;
    
    __m128i vecA = _mm_set1_epi8('A');
    __m128i vecC = _mm_set1_epi8('C');
    __m128i vecG = _mm_set1_epi8('G');
    __m128i vecT = _mm_set1_epi8('T');
    
    size_t i = 0;
    for (; i + 16 <= size; i += 16) {
        __m128i chunk = _mm_loadu_si128((const __m128i*)(buffer + i));
        
        __m128i cmpA = _mm_cmpeq_epi8(chunk, vecA);
        __m128i cmpC = _mm_cmpeq_epi8(chunk, vecC);
        __m128i cmpG = _mm_cmpeq_epi8(chunk, vecG);
        __m128i cmpT = _mm_cmpeq_epi8(chunk, vecT);
        
        int maskA = _mm_movemask_epi8(cmpA);
        int maskC = _mm_movemask_epi8(cmpC);
        int maskG = _mm_movemask_epi8(cmpG);
        int maskT = _mm_movemask_epi8(cmpT);
        
        counts[0] += __builtin_popcount(maskA);
        counts[1] += __builtin_popcount(maskC);
        counts[2] += __builtin_popcount(maskG);
        counts[3] += __builtin_popcount(maskT);
    }
    
    for (; i < size; i++) {
        char c = buffer[i];
        if (c == 'A') counts[0]++;
        else if (c == 'C') counts[1]++;
        else if (c == 'G') counts[2]++;
        else if (c == 'T') counts[3]++;
    }
#endif

void* thread_count_scalar(void *arg) {
    ThreadArgs *args = (ThreadArgs*)arg;
    unsigned long long local_counts[4] = {0};
    
    for (size_t i = args->start; i < args->end; i++) {
        char c = args->buffer[i];
        if (c == 'A') local_counts[0]++;
        else if (c == 'C') local_counts[1]++;
        else if (c == 'G') local_counts[2]++;
        else if (c == 'T') local_counts[3]++;
    }
    
    for (int j = 0; j < 4; j++) {
        args->counts[j] = local_counts[j];
    }
    
    return NULL;
}

void* thread_count_simd(void *arg) {
    ThreadArgs *args = (ThreadArgs*)arg;
    unsigned long long local_counts[4] = {0};
    size_t chunk_size = args->end - args->start;
    
    count_simd(args->buffer + args->start, chunk_size, local_counts);
    
    for (int j = 0; j < 4; j++) {
        args->counts[j] = local_counts[j];
    }
    
    return NULL;
}

void count_multithread_scalar(const char *buffer, size_t size, int num_threads, unsigned long long result[4]) {
    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];
    size_t chunk_size = size / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        args[i].buffer = (char*)buffer;
        args[i].start = i * chunk_size;
        args[i].end = (i == num_threads - 1) ? size : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, thread_count_scalar, &args[i]);
    }
    
    for (int i = 0; i < 4; i++) result[i] = 0;
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        for (int j = 0; j < 4; j++) {
            result[j] += args[i].counts[j];
        }
    }
}

void count_multithread_simd(const char *buffer, size_t size, int num_threads, unsigned long long result[4]) {
    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];
    size_t chunk_size = size / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        args[i].buffer = (char*)buffer;
        args[i].start = i * chunk_size;
        args[i].end = (i == num_threads - 1) ? size : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, thread_count_simd, &args[i]);
    }
    
    for (int i = 0; i < 4; i++) result[i] = 0;
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        for (int j = 0; j < 4; j++) {
            result[j] += args[i].counts[j];
        }
    }
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

int results_match(unsigned long long r1[4], unsigned long long r2[4]) {
    for (int i = 0; i < 4; i++) {
        if (r1[i] != r2[i]) return 0;
    }
    return 1;
}

int main() {
    char *dna = (char*)malloc(BUFFER_SIZE);
    if (!dna) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    printf("Generating DNA sequence of size: %.2f MB\n", BUFFER_SIZE / (1024.0 * 1024.0));
    srand(42);
    generate_dna(dna, BUFFER_SIZE);
    
    unsigned long long scalar_counts[4], simd_counts[4];
    unsigned long long mt_scalar_counts[4], mt_simd_counts[4];
    double start, end;
    double scalar_time, simd_time, mt_scalar_time, mt_simd_time;
    
    printf(".    RESULTS\n");
    printf("Buffer size: %lu bytes (%.2f MB)\n", (unsigned long)BUFFER_SIZE, BUFFER_SIZE / (1024.0 * 1024.0));
    printf("Threads: %d\n\n", NUM_THREADS);
    
    start = get_time();
    count_scalar(dna, BUFFER_SIZE, scalar_counts);
    end = get_time();
    scalar_time = end - start;
    printf("Scalar:                %.4f sec  (A:%llu C:%llu G:%llu T:%llu)\n", 
           scalar_time, scalar_counts[0], scalar_counts[1], scalar_counts[2], scalar_counts[3]);
    
    start = get_time();
    count_simd(dna, BUFFER_SIZE, simd_counts);
    end = get_time();
    simd_time = end - start;
    printf("SIMD:                  %.4f sec  (A:%llu C:%llu G:%llu T:%llu)\n", 
           simd_time, simd_counts[0], simd_counts[1], simd_counts[2], simd_counts[3]);
    
    start = get_time();
    count_multithread_scalar(dna, BUFFER_SIZE, NUM_THREADS, mt_scalar_counts);
    end = get_time();
    mt_scalar_time = end - start;
    printf("Multithread Scalar:    %.4f sec  (A:%llu C:%llu G:%llu T:%llu)\n", 
           mt_scalar_time, mt_scalar_counts[0], mt_scalar_counts[1], mt_scalar_counts[2], mt_scalar_counts[3]);
    
    start = get_time();
    count_multithread_simd(dna, BUFFER_SIZE, NUM_THREADS, mt_simd_counts);
    end = get_time();
    mt_simd_time = end - start;
    printf("Multithread SIMD:      %.4f sec  (A:%llu C:%llu G:%llu T:%llu)\n", 
           mt_simd_time, mt_simd_counts[0], mt_simd_counts[1], mt_simd_counts[2], mt_simd_counts[3]);
    
    free(dna);
    return 0;
}
