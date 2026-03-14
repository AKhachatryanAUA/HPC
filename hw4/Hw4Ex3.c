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

#define NUM_THREADS 4
#define MAX_LINE_LENGTH 256

typedef struct {
    uint8_t *input_buffer;
    uint8_t *output_buffer;
    int width;
    int height;
    int channels;
    int start_row;
    int end_row;
} ThreadArgs;

typedef struct {
    char magic[3];
    int width;
    int height;
    int max_val;
    size_t data_size;
    uint8_t *data;
} PPMImage;

void skip_comments(FILE *file) {
    int c;
    while ((c = fgetc(file)) == '#') {
        while (fgetc(file) != '\n');
    }
    ungetc(c, file);
}

PPMImage* read_ppm(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    PPMImage *img = (PPMImage*)malloc(sizeof(PPMImage));
    if (!img) {
        fclose(file);
        return NULL;
    }
    
    fscanf(file, "%2s", img->magic);
    skip_comments(file);
    fscanf(file, "%d %d", &img->width, &img->height);
    skip_comments(file);
    fscanf(file, "%d", &img->max_val);
    fgetc(file);
    
    img->data_size = img->width * img->height * 3;
    img->data = (uint8_t*)malloc(img->data_size);
    
    if (!img->data) {
        free(img);
        fclose(file);
        return NULL;
    }
    
    if (strcmp(img->magic, "P6") == 0) {
        fread(img->data, 1, img->data_size, file);
    } else if (strcmp(img->magic, "P3") == 0) {
        for (size_t i = 0; i < img->data_size; i++) {
            int val;
            fscanf(file, "%d", &val);
            img->data[i] = (uint8_t)val;
        }
    } else {
        printf("Error: Unsupported PPM format %s\n", img->magic);
        free(img->data);
        free(img);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    return img;
}

int write_ppm(const char *filename, PPMImage *img) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Error: Cannot create file %s\n", filename);
        return 0;
    }
    
    fprintf(file, "P6\n");
    fprintf(file, "%d %d\n", img->width, img->height);
    fprintf(file, "%d\n", img->max_val);
    
    fwrite(img->data, 1, img->data_size, file);
    
    fclose(file);
    return 1;
}

void free_ppm(PPMImage *img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}

PPMImage* copy_ppm_image(PPMImage *src) {
    PPMImage *dst = (PPMImage*)malloc(sizeof(PPMImage));
    if (!dst) return NULL;
    
    memcpy(dst, src, sizeof(PPMImage));
    dst->data = (uint8_t*)malloc(src->data_size);
    
    if (!dst->data) {
        free(dst);
        return NULL;
    }
    
    memcpy(dst->data, src->data, src->data_size);
    return dst;
}

void convert_scalar(PPMImage *img) {
    uint8_t *data = img->data;
    int num_pixels = img->width * img->height;
    
    for (int i = 0; i < num_pixels; i++) {
        int r = data[i*3];
        int g = data[i*3 + 1];
        int b = data[i*3 + 2];
        
        int gray = (int)(0.299f * r + 0.587f * g + 0.114f * b);
        if (gray > 255) gray = 255;
        
        data[i*3] = gray;
        data[i*3 + 1] = gray;
        data[i*3 + 2] = gray;
    }
}

#if defined(USING_AVX)
void convert_simd(PPMImage *img) {
    uint8_t *data = img->data;
    int num_pixels = img->width * img->height;
    int i = 0;
    
    __m256 scale_r = _mm256_set1_ps(0.299f);
    __m256 scale_g = _mm256_set1_ps(0.587f);
    __m256 scale_b = _mm256_set1_ps(0.114f);
    
    for (; i + 8 <= num_pixels; i += 8) {
        __m128i pixels_low = _mm_loadu_si128((__m128i*)(data + i*3));
        __m128i pixels_high = _mm_loadu_si128((__m128i*)(data + i*3 + 16));
        
        __m128i r = _mm_setzero_si128();
        __m128i g = _mm_setzero_si128();
        __m128i b = _mm_setzero_si128();
        
        for (int j = 0; j < 8; j++) {
            r = _mm_insert_epi16(r, data[(i+j)*3], j);
            g = _mm_insert_epi16(g, data[(i+j)*3 + 1], j);
            b = _mm_insert_epi16(b, data[(i+j)*3 + 2], j);
        }
        
        __m256 r_float = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(r));
        __m256 g_float = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(g));
        __m256 b_float = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(b));
        
        __m256 gray_float = _mm256_add_ps(
            _mm256_add_ps(
                _mm256_mul_ps(r_float, scale_r),
                _mm256_mul_ps(g_float, scale_g)
            ),
            _mm256_mul_ps(b_float, scale_b)
        );
        
        __m256i gray_int = _mm256_cvtps_epi32(gray_float);
        
        for (int j = 0; j < 8; j++) {
            uint8_t g_val = (uint8_t)_mm256_extract_epi32(gray_int, j);
            data[(i+j)*3] = g_val;
            data[(i+j)*3 + 1] = g_val;
            data[(i+j)*3 + 2] = g_val;
        }
    }
    
    for (; i < num_pixels; i++) {
        int r = data[i*3];
        int g = data[i*3 + 1];
        int b = data[i*3 + 2];
        
        int gray = (int)(0.299f * r + 0.587f * g + 0.114f * b);
        if (gray > 255) gray = 255;
        
        data[i*3] = gray;
        data[i*3 + 1] = gray;
        data[i*3 + 2] = gray;
    }
}

#elif defined(USING_NEON)
void convert_simd(PPMImage *img) {
    uint8_t *data = img->data;
    int num_pixels = img->width * img->height;
    int i = 0;
    
    float32x4_t scale_r = vdupq_n_f32(0.299f);
    float32x4_t scale_g = vdupq_n_f32(0.587f);
    float32x4_t scale_b = vdupq_n_f32(0.114f);
    
    for (; i + 4 <= num_pixels; i += 4) {
        uint8x8x3_t rgb = vld3_u8(data + i*3);
        
        uint16x8_t r16 = vmovl_u8(rgb.val[0]);
        uint16x8_t g16 = vmovl_u8(rgb.val[1]);
        uint16x8_t b16 = vmovl_u8(rgb.val[2]);
        
        uint32x4_t r_low = vmovl_u16(vget_low_u16(r16));
        uint32x4_t r_high = vmovl_u16(vget_high_u16(r16));
        uint32x4_t g_low = vmovl_u16(vget_low_u16(g16));
        uint32x4_t g_high = vmovl_u16(vget_high_u16(g16));
        uint32x4_t b_low = vmovl_u16(vget_low_u16(b16));
        uint32x4_t b_high = vmovl_u16(vget_high_u16(b16));
        
        float32x4_t r_float_low = vcvtq_f32_u32(r_low);
        float32x4_t r_float_high = vcvtq_f32_u32(r_high);
        float32x4_t g_float_low = vcvtq_f32_u32(g_low);
        float32x4_t g_float_high = vcvtq_f32_u32(g_high);
        float32x4_t b_float_low = vcvtq_f32_u32(b_low);
        float32x4_t b_float_high = vcvtq_f32_u32(b_high);
        
        float32x4_t gray_low = vaddq_f32(
            vaddq_f32(
                vmulq_f32(r_float_low, scale_r),
                vmulq_f32(g_float_low, scale_g)
            ),
            vmulq_f32(b_float_low, scale_b)
        );
        
        float32x4_t gray_high = vaddq_f32(
            vaddq_f32(
                vmulq_f32(r_float_high, scale_r),
                vmulq_f32(g_float_high, scale_g)
            ),
            vmulq_f32(b_float_high, scale_b)
        );
        
        uint32x4_t gray_int_low = vcvtq_u32_f32(gray_low);
        uint32x4_t gray_int_high = vcvtq_u32_f32(gray_high);
        
        uint16x4_t gray_low16 = vmovn_u32(gray_int_low);
        uint16x4_t gray_high16 = vmovn_u32(gray_int_high);
        uint16x8_t gray16 = vcombine_u16(gray_low16, gray_high16);
        
        uint8x8_t gray8 = vmovn_u16(gray16);
        
        rgb.val[0] = gray8;
        rgb.val[1] = gray8;
        rgb.val[2] = gray8;
        
        vst3_u8(data + i*3, rgb);
    }
    
    for (; i < num_pixels; i++) {
        int r = data[i*3];
        int g = data[i*3 + 1];
        int b = data[i*3 + 2];
        
        int gray = (int)(0.299f * r + 0.587f * g + 0.114f * b);
        if (gray > 255) gray = 255;
        
        data[i*3] = gray;
        data[i*3 + 1] = gray;
        data[i*3 + 2] = gray;
    }
}

#else
void convert_simd(PPMImage *img) {
    uint8_t *data = img->data;
    int num_pixels = img->width * img->height;
    int i = 0;
    
    __m128 scale_r = _mm_set1_ps(0.299f);
    __m128 scale_g = _mm_set1_ps(0.587f);
    __m128 scale_b = _mm_set1_ps(0.114f);
    
    for (; i + 4 <= num_pixels; i += 4) {
        uint8_t r[4], g[4], b[4];
        for (int j = 0; j < 4; j++) {
            r[j] = data[(i+j)*3];
            g[j] = data[(i+j)*3 + 1];
            b[j] = data[(i+j)*3 + 2];
        }
        
        __m128i r_i128 = _mm_loadu_si128((__m128i*)r);
        __m128i g_i128 = _mm_loadu_si128((__m128i*)g);
        __m128i b_i128 = _mm_loadu_si128((__m128i*)b);
        
        __m128i r_i32 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(r_i128, _mm_setzero_si128()), _mm_setzero_si128());
        __m128i g_i32 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(g_i128, _mm_setzero_si128()), _mm_setzero_si128());
        __m128i b_i32 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(b_i128, _mm_setzero_si128()), _mm_setzero_si128());
        
        __m128 r_float = _mm_cvtepi32_ps(r_i32);
        __m128 g_float = _mm_cvtepi32_ps(g_i32);
        __m128 b_float = _mm_cvtepi32_ps(b_i32);
        
        __m128 gray_float = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(r_float, scale_r),
                _mm_mul_ps(g_float, scale_g)
            ),
            _mm_mul_ps(b_float, scale_b)
        );
        
        __m128i gray_int = _mm_cvtps_epi32(gray_float);
        
        for (int j = 0; j < 4; j++) {
            uint8_t g_val = (uint8_t)_mm_extract_epi16(gray_int, j*2);
            data[(i+j)*3] = g_val;
            data[(i+j)*3 + 1] = g_val;
            data[(i+j)*3 + 2] = g_val;
        }
    }
    
    for (; i < num_pixels; i++) {
        int r = data[i*3];
        int g = data[i*3 + 1];
        int b = data[i*3 + 2];
        
        int gray = (int)(0.299f * r + 0.587f * g + 0.114f * b);
        if (gray > 255) gray = 255;
        
        data[i*3] = gray;
        data[i*3 + 1] = gray;
        data[i*3 + 2] = gray;
    }
}
#endif

void* thread_convert_scalar(void *arg) {
    ThreadArgs *args = (ThreadArgs*)arg;
    uint8_t *input = args->input_buffer;
    uint8_t *output = args->output_buffer;
    int width = args->width;
    int channels = args->channels;
    int row_stride = width * channels;
    
    for (int row = args->start_row; row < args->end_row; row++) {
        uint8_t *row_input = input + row * row_stride;
        uint8_t *row_output = output + row * row_stride;
        
        for (int col = 0; col < width; col++) {
            int r = row_input[col*channels];
            int g = row_input[col*channels + 1];
            int b = row_input[col*channels + 2];
            
            int gray = (int)(0.299f * r + 0.587f * g + 0.114f * b);
            if (gray > 255) gray = 255;
            
            row_output[col*channels] = gray;
            row_output[col*channels + 1] = gray;
            row_output[col*channels + 2] = gray;
        }
    }
    
    return NULL;
}

void convert_multithread(PPMImage *img, int num_threads) {
    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];
    int rows_per_thread = img->height / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        args[i].input_buffer = img->data;
        args[i].output_buffer = img->data;
        args[i].width = img->width;
        args[i].height = img->height;
        args[i].channels = 3;
        args[i].start_row = i * rows_per_thread;
        args[i].end_row = (i == num_threads - 1) ? img->height : (i + 1) * rows_per_thread;
        
        pthread_create(&threads[i], NULL, thread_convert_scalar, &args[i]);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

void* thread_convert_simd(void *arg) {
    ThreadArgs *args = (ThreadArgs*)arg;
    uint8_t *input = args->input_buffer;
    uint8_t *output = args->output_buffer;
    int width = args->width;
    int channels = args->channels;
    int row_stride = width * channels;
    
    for (int row = args->start_row; row < args->end_row; row++) {
        uint8_t *row_input = input + row * row_stride;
        uint8_t *row_output = output + row * row_stride;
        int num_pixels = width;
        int i = 0;
        
#if defined(USING_AVX)
        __m256 scale_r = _mm256_set1_ps(0.299f);
        __m256 scale_g = _mm256_set1_ps(0.587f);
        __m256 scale_b = _mm256_set1_ps(0.114f);
        
        for (; i + 8 <= num_pixels; i += 8) {
            __m128i r = _mm_setzero_si128();
            __m128i g = _mm_setzero_si128();
            __m128i b = _mm_setzero_si128();
            
            for (int j = 0; j < 8; j++) {
                r = _mm_insert_epi16(r, row_input[(i+j)*channels], j);
                g = _mm_insert_epi16(g, row_input[(i+j)*channels + 1], j);
                b = _mm_insert_epi16(b, row_input[(i+j)*channels + 2], j);
            }
            
            __m256 r_float = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(r));
            __m256 g_float = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(g));
            __m256 b_float = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(b));
            
            __m256 gray_float = _mm256_add_ps(
                _mm256_add_ps(
                    _mm256_mul_ps(r_float, scale_r),
                    _mm256_mul_ps(g_float, scale_g)
                ),
                _mm256_mul_ps(b_float, scale_b)
            );
            
            __m256i gray_int = _mm256_cvtps_epi32(gray_float);
            
            for (int j = 0; j < 8; j++) {
                uint8_t g_val = (uint8_t)_mm256_extract_epi32(gray_int, j);
                row_output[(i+j)*channels] = g_val;
                row_output[(i+j)*channels + 1] = g_val;
                row_output[(i+j)*channels + 2] = g_val;
            }
        }
#elif defined(USING_NEON)
        float32x4_t scale_r = vdupq_n_f32(0.299f);
        float32x4_t scale_g = vdupq_n_f32(0.587f);
        float32x4_t scale_b = vdupq_n_f32(0.114f);
        
        for (; i + 4 <= num_pixels; i += 4) {
            uint8x8x3_t rgb = vld3_u8(row_input + i*channels);
            
            uint16x8_t r16 = vmovl_u8(rgb.val[0]);
            uint16x8_t g16 = vmovl_u8(rgb.val[1]);
            uint16x8_t b16 = vmovl_u8(rgb.val[2]);
            
            uint32x4_t r_low = vmovl_u16(vget_low_u16(r16));
            uint32x4_t r_high = vmovl_u16(vget_high_u16(r16));
            uint32x4_t g_low = vmovl_u16(vget_low_u16(g16));
            uint32x4_t g_high = vmovl_u16(vget_high_u16(g16));
            uint32x4_t b_low = vmovl_u16(vget_low_u16(b16));
            uint32x4_t b_high = vmovl_u16(vget_high_u16(b16));
            
            float32x4_t r_float_low = vcvtq_f32_u32(r_low);
            float32x4_t r_float_high = vcvtq_f32_u32(r_high);
            float32x4_t g_float_low = vcvtq_f32_u32(g_low);
            float32x4_t g_float_high = vcvtq_f32_u32(g_high);
            float32x4_t b_float_low = vcvtq_f32_u32(b_low);
            float32x4_t b_float_high = vcvtq_f32_u32(b_high);
            
            float32x4_t gray_low = vaddq_f32(
                vaddq_f32(
                    vmulq_f32(r_float_low, scale_r),
                    vmulq_f32(g_float_low, scale_g)
                ),
                vmulq_f32(b_float_low, scale_b)
            );
            
            float32x4_t gray_high = vaddq_f32(
                vaddq_f32(
                    vmulq_f32(r_float_high, scale_r),
                    vmulq_f32(g_float_high, scale_g)
                ),
                vmulq_f32(b_float_high, scale_b)
            );
            
            uint32x4_t gray_int_low = vcvtq_u32_f32(gray_low);
            uint32x4_t gray_int_high = vcvtq_u32_f32(gray_high);
            
            uint16x4_t gray_low16 = vmovn_u32(gray_int_low);
            uint16x4_t gray_high16 = vmovn_u32(gray_int_high);
            uint16x8_t gray16 = vcombine_u16(gray_low16, gray_high16);
            
            uint8x8_t gray8 = vmovn_u16(gray16);
            
            rgb.val[0] = gray8;
            rgb.val[1] = gray8;
            rgb.val[2] = gray8;
            
            vst3_u8(row_output + i*channels, rgb);
        }
#else
        __m128 scale_r = _mm_set1_ps(0.299f);
        __m128 scale_g = _mm_set1_ps(0.587f);
        __m128 scale_b = _mm_set1_ps(0.114f);
        
        for (; i + 4 <= num_pixels; i += 4) {
            uint8_t r[4], g[4], b[4];
            for (int j = 0; j < 4; j++) {
                r[j] = row_input[(i+j)*channels];
                g[j] = row_input[(i+j)*channels + 1];
                b[j] = row_input[(i+j)*channels + 2];
            }
            
            __m128i r_i128 = _mm_loadu_si128((__m128i*)r);
            __m128i g_i128 = _mm_loadu_si128((__m128i*)g);
            __m128i b_i128 = _mm_loadu_si128((__m128i*)b);
            
            __m128i r_i32 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(r_i128, _mm_setzero_si128()), _mm_setzero_si128());
            __m128i g_i32 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(g_i128, _mm_setzero_si128()), _mm_setzero_si128());
            __m128i b_i32 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(b_i128, _mm_setzero_si128()), _mm_setzero_si128());
            
            __m128 r_float = _mm_cvtepi32_ps(r_i32);
            __m128 g_float = _mm_cvtepi32_ps(g_i32);
            __m128 b_float = _mm_cvtepi32_ps(b_i32);
            
            __m128 gray_float = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(r_float, scale_r),
                    _mm_mul_ps(g_float, scale_g)
                ),
                _mm_mul_ps(b_float, scale_b)
            );
            
            __m128i gray_int = _mm_cvtps_epi32(gray_float);
            
            for (int j = 0; j < 4; j++) {
                uint8_t g_val = (uint8_t)_mm_extract_epi16(gray_int, j*2);
                row_output[(i+j)*channels] = g_val;
                row_output[(i+j)*channels + 1] = g_val;
                row_output[(i+j)*channels + 2] = g_val;
            }
        }
#endif
        
        for (; i < num_pixels; i++) {
            int r = row_input[i*channels];
            int g = row_input[i*channels + 1];
            int b = row_input[i*channels + 2];
            
            int gray = (int)(0.299f * r + 0.587f * g + 0.114f * b);
            if (gray > 255) gray = 255;
            
            row_output[i*channels] = gray;
            row_output[i*channels + 1] = gray;
            row_output[i*channels + 2] = gray;
        }
    }
    
    return NULL;
}

void convert_multithread_simd(PPMImage *img, int num_threads) {
    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];
    int rows_per_thread = img->height / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        args[i].input_buffer = img->data;
        args[i].output_buffer = img->data;
        args[i].width = img->width;
        args[i].height = img->height;
        args[i].channels = 3;
        args[i].start_row = i * rows_per_thread;
        args[i].end_row = (i == num_threads - 1) ? img->height : (i + 1) * rows_per_thread;
        
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

int verify_images(PPMImage *img1, PPMImage *img2) {
    if (img1->width != img2->width || img1->height != img2->height) {
        return 0;
    }
    
    size_t data_size = img1->width * img1->height * 3;
    return memcmp(img1->data, img2->data, data_size) == 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_ppm> <output_ppm>\n", argv[0]);
        return 1;
    }
    
    printf("Reading image: %s\n", argv[1]);
    PPMImage *original = read_ppm(argv[1]);
    if (!original) {
        printf("Failed to read input image\n");
        return 1;
    }
    
    printf("Image size: %d x %d\n", original->width, original->height);
    printf("Format: %s\n", original->magic);
    printf("Max value: %d\n", original->max_val);
    
    PPMImage *img_scalar = copy_ppm_image(original);
    PPMImage *img_simd = copy_ppm_image(original);
    PPMImage *img_multithread = copy_ppm_image(original);
    PPMImage *img_multithread_simd = copy_ppm_image(original);
    
    if (!img_scalar || !img_simd || !img_multithread || !img_multithread_simd) {
        printf("Failed to allocate image copies\n");
        free_ppm(original);
        return 1;
    }
    
    double start, end;
    double time_scalar, time_simd, time_multithread, time_multithread_simd;
    
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
    
    printf("\n=== PROCESSING WITH %d THREADS ===\n\n", NUM_THREADS);
    
    start = get_time();
    convert_scalar(img_scalar);
    end = get_time();
    time_scalar = end - start;
    printf("Scalar time:               %.4f sec\n", time_scalar);
    
    start = get_time();
    convert_simd(img_simd);
    end = get_time();
    time_simd = end - start;
    printf("SIMD time:                 %.4f sec\n", time_simd);
    
    start = get_time();
    convert_multithread(img_multithread, NUM_THREADS);
    end = get_time();
    time_multithread = end - start;
    printf("Multithreading time:       %.4f sec\n", time_multithread);
    
    start = get_time();
    convert_multithread_simd(img_multithread_simd, NUM_THREADS);
    end = get_time();
    time_multithread_simd = end - start;
    printf("Multithreading + SIMD time: %.4f sec\n", time_multithread_simd);
    
    int simd_valid = verify_images(img_scalar, img_simd);
    int mt_valid = verify_images(img_scalar, img_multithread);
    int mt_simd_valid = verify_images(img_scalar, img_multithread_simd);
    
    int all_valid = simd_valid && mt_valid && mt_simd_valid;
    
    printf("Overall:                  %s\n", all_valid ? "PASSED" : "FAILED");
    
    printf("\nSaving output image: %s\n", argv[2]);
    if (write_ppm(argv[2], img_multithread_simd)) {
        printf("Output saved successfully\n");
    } else {
        printf("Failed to save output image\n");
    }
    
    free_ppm(original);
    free_ppm(img_scalar);
    free_ppm(img_simd);
    free_ppm(img_multithread);
    free_ppm(img_multithread_simd);
    
    return 0;
}