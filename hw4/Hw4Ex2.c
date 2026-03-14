#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
    #define USING_NEON 1
#elif defined(__AVX2__) || defined(__AVX__)
    #include <immintrin.h>
    #define USING_AVX 1
#else
    #include <emmintrin.h>
    #define USING_SSE 1
#endif

#define BUFFER_SIZE (256 * 1024 * 1024)
#define NUM_THREADS 4

typedef struct {
    char *buffer;
    size_t start;
    size_t end;
} ThreadArgs;

void generate_buffer(char *buffer, size_t size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+ {}[]:;\"'<>,.?/ \t\n";
    int charset_size = sizeof(charset) - 1;
    
    for (size_t i = 0; i < size; i++) {
        buffer[i] = charset[rand() % charset_size];
    }
}

char* copy_buffer(const char *source, size_t size) {
    char *dest = (char*)malloc(size);
    if (dest) {
        memcpy(dest, source, size);
    }
    return dest;
}

int verify_buffers(const char *buf1, const char *buf2, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (buf1[i] != buf2[i]) return 0;
    }
    return 1;
}

void convert_scalar(char *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (buffer[i] >= 'a' && buffer[i] <= 'z') {
            buffer[i] = buffer[i] - 32;
        }
    }
}

void* thread_convert_scalar(void *arg) {
    ThreadArgs *args = (ThreadArgs*)arg;
    
    for (size_t i = args->start; i < args->end; i++) {
        if (args->buffer[i] >= 'a' && args->buffer[i] <= 'z') {
            args->buffer[i] = args->buffer[i] - 32;
        }
    }
    
    return NULL;
}

void convert_multithread(char *buffer, size_t size, int num_threads) {
    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];
    size_t chunk_size = size / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        args[i].buffer = buffer;
        args[i].start = i * chunk_size;
        args[i].end = (i == num_threads - 1) ? size : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, thread_convert_scalar, &args[i]);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

#if defined(USING_NEON)
void convert_simd(char *buffer, size_t size) {
    size_t i = 0;
    
    for (; i + 16 <= size; i += 16) {
        uint8x16_t chars = vld1q_u8((uint8_t*)(buffer + i));
        
        uint8x16_t vec_a = vdupq_n_u8('a');
        uint8x16_t vec_z = vdupq_n_u8('z');
        uint8x16_t vec_32 = vdupq_n_u8(32);
        
        uint8x16_t ge_a = vcgeq_u8(chars, vec_a);
        uint8x16_t le_z = vcleq_u8(chars, vec_z);
        
        uint8x16_t is_lower = vandq_u8(ge_a, le_z);
        uint8x16_t mask = vandq_u8(is_lower, vec_32);
        uint8x16_t result = vsubq_u8(chars, mask);
        
        vst1q_u8((uint8_t*)(buffer + i), result);
    }
    
    for (; i < size; i++) {
        if (buffer[i] >= 'a' && buffer[i] <= 'z') {
            buffer[i] = buffer[i] - 32;
        }
    }
}

#elif defined(USING_AVX)
void convert_simd(char *buffer, size_t size) {
    size_t i = 0;
    
    for (; i + 32 <= size; i += 32) {
        __m256i chars = _mm256_loadu_si256((__m256i*)(buffer + i));
        
        __m256i vec_a = _mm256_set1_epi8('a');
        __m256i vec_z = _mm256_set1_epi8('z');
        __m256i vec_32 = _mm256_set1_epi8(32);
        
        __m256i ge_a = _mm256_cmpgt_epi8(chars, _mm256_set1_epi8('a' - 1));
        __m256i le_z = _mm256_cmpgt_epi8(_mm256_set1_epi8('z' + 1), chars);
        
        __m256i is_lower = _mm256_and_si256(ge_a, le_z);
        __m256i mask = _mm256_and_si256(is_lower, vec_32);
        __m256i result = _mm256_sub_epi8(chars, mask);
        
        _mm256_storeu_si256((__m256i*)(buffer + i), result);
    }
    
    for (; i < size; i++) {
        if (buffer[i] >= 'a' && buffer[i] <= 'z') {
            buffer[i] = buffer[i] - 32;
        }
    }
}

#else
void convert_simd(char *buffer, size_t size) {
    size_t i = 0;
    
    for (; i + 16 <= size; i += 16) {
        __m128i chars = _mm_loadu_si128((__m128i*)(buffer + i));
        
        __m128i vec_a = _mm_set1_epi8('a');
        __m128i vec_z = _mm_set1_epi8('z');
        __m128i vec_32 = _mm_set1_epi8(32);
        
        __m128i ge_a = _mm_cmpgt_epi8(chars, _mm_set1_epi8('a' - 1));
        __m128i le_z = _mm_cmpgt_epi8(_mm_set1_epi8('z' + 1), chars);
        
        __m128i is_lower = _mm_and_si128(ge_a, le_z);
        __m128i mask = _mm_and_si128(is_lower, vec_32);
        __m128i result = _mm_sub_epi8(chars, mask);
        
        _mm_storeu_si128((__m128i*)(buffer + i), result);
    }
    
    for (; i < size; i++) {
        if (buffer[i] >= 'a' && buffer[i] <= 'z') {
            buffer[i] = buffer[i] - 32;
        }
    }
}
#endif

void* thread_convert_simd(void *arg) {
    ThreadArgs *args = (ThreadArgs*)arg;
    size_t chunk_size = args->end - args->start;
    
    convert_simd(args->buffer + args->start, chunk_size);
    
    return NULL;
}

void convert_multithread_simd(char *buffer, size_t size, int num_threads) {
    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];
    size_t chunk_size = size / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        args[i].buffer = buffer;
        args[i].start = i * chunk_size;
        args[i].end = (i == num_threads - 1) ? size : (i + 1) * chunk_size;
        pthread_create(&threads[i], NULL, thread_convert_simd, &args[i]);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

void print_sample(const char *buffer, size_t size, const char *label) {
    printf("%s: \"", label);
    size_t sample_size = (size < 50) ? size : 50;
    for (size_t i = 0; i < sample_size; i++) {
        char c = buffer[i];
        if (c == '\n') printf("\\n");
        else if (c == '\t') printf("\\t");
        else printf("%c", c);
    }
    printf("%s\"\n", size > 50 ? "..." : "");
}

int main() {
    char *original = (char*)malloc(BUFFER_SIZE);
    if (!original) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    printf("Generating random buffer of size: %.2f MB\n", BUFFER_SIZE / (1024.0 * 1024.0));
    srand(time(NULL));
    generate_buffer(original, BUFFER_SIZE);
    
    char *buffer_mt = copy_buffer(original, BUFFER_SIZE);
    char *buffer_simd = copy_buffer(original, BUFFER_SIZE);
    char *buffer_mt_simd = copy_buffer(original, BUFFER_SIZE);
    char *buffer_scalar = copy_buffer(original, BUFFER_SIZE);
    
    if (!buffer_mt || !buffer_simd || !buffer_mt_simd || !buffer_scalar) {
        printf("Failed to allocate copy buffers\n");
        free(original);
        return 1;
    }
    
    double start, end;
    double time_mt, time_simd, time_mt_simd;
    
    printf("\n=== INPUT SAMPLE ===\n");
    print_sample(original, BUFFER_SIZE, "Original");
    
    printf("\n=== PROCESSING %d MB WITH %d THREADS ===\n", 
           (int)(BUFFER_SIZE / (1024 * 1024)), NUM_THREADS);
    
    printf("\nSIMD Implementation: ");
#if defined(USING_NEON)
    printf("NEON (ARM/Apple Silicon)\n");
#elif defined(USING_AVX)
    printf("AVX2 (Intel)\n");
#elif defined(USING_SSE)
    printf("SSE2 (Fallback)\n");
#else
    printf("None\n");
#endif
    
    convert_scalar(buffer_scalar, BUFFER_SIZE);
    
    start = get_time();
    convert_multithread(buffer_mt, BUFFER_SIZE, NUM_THREADS);
    end = get_time();
    time_mt = end - start;
    
    start = get_time();
    convert_simd(buffer_simd, BUFFER_SIZE);
    end = get_time();
    time_simd = end - start;
    
    start = get_time();
    convert_multithread_simd(buffer_mt_simd, BUFFER_SIZE, NUM_THREADS);
    end = get_time();
    time_mt_simd = end - start;
    
    int mt_valid = verify_buffers(buffer_scalar, buffer_mt, BUFFER_SIZE);
    int simd_valid = verify_buffers(buffer_scalar, buffer_simd, BUFFER_SIZE);
    int mt_simd_valid = verify_buffers(buffer_scalar, buffer_mt_simd, BUFFER_SIZE);
    
    printf("\n=== RESULTS ===\n");
    printf("Buffer size: %d MB\n", (int)(BUFFER_SIZE / (1024 * 1024)));
    printf("Threads used: %d\n\n", NUM_THREADS);
    
    printf("Multithreading time:      %.4f sec\n", 
           time_mt);
    printf("SIMD time:                %.4f sec\n", 
           time_simd);
    printf("SIMD + Multithreading:    %.4f sec\n", 
           time_mt_simd);
    
    printf("\n=== OUTPUT SAMPLE ===\n");
    print_sample(buffer_mt_simd, BUFFER_SIZE, "Converted");
    
    free(original);
    free(buffer_mt);
    free(buffer_simd);
    free(buffer_mt_simd);
    free(buffer_scalar);
    
    return 0;
}