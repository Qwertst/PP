#include <iostream>
#include <immintrin.h>
#include <cstring>
#include <iomanip>
#include <random>

const size_t VECTOR_SIZE = 10000000;

void fill(float *arr) {
    static std::default_random_engine gen;
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (size_t i = 0; i < VECTOR_SIZE; ++i) {
        arr[i] = dist(gen);
    }

}

__attribute__((noinline))
void add_vectors_naive(const float* a, const float* b, float* result, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        result[i] = a[i] + b[i];
    }
}

__attribute__((noinline))
void add_vectors_sse(const float* a, const float* b, float* result, size_t size) {
    size_t i = 0;

    for (; i + 4 <= size; i += 4) {
        __m128 va = _mm_loadu_ps(&a[i]);
        __m128 vb = _mm_loadu_ps(&b[i]);
        __m128 vresult = _mm_add_ps(va, vb);
        _mm_storeu_ps(&result[i], vresult);
    }

    for (; i < size; ++i) {
        result[i] = a[i] + b[i];
    }
}


__attribute__((noinline))
void add_vectors_avx(const float* a, const float* b, float* result, size_t size) {
    size_t i = 0;

    for (; i + 8 <= size; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vresult = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(&result[i], vresult);
    }

    for (; i < size; ++i) {
        result[i] = a[i] + b[i];
    }
}

inline uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

template<typename Func>
double benchmark_rdtsc(Func func, const float* a, const float* b, float* result,
                       size_t size, int iterations = 1000) {
    uint64_t total_cycles = 0;

    for (int i = 0; i < 10; ++i) {
        func(a, b, result, size);
    }

    for (int i = 0; i < iterations; ++i) {
        uint64_t start = rdtsc();
        func(a, b, result, size);
        uint64_t end = rdtsc();
        total_cycles += (end - start);
    }

    return static_cast<double>(total_cycles) / iterations;
}

int main() {
    float* a = static_cast<float*>(aligned_alloc(32, VECTOR_SIZE * sizeof(float)));
    float* b = static_cast<float*>(aligned_alloc(32, VECTOR_SIZE * sizeof(float)));
    float* result = static_cast<float*>(aligned_alloc(32, VECTOR_SIZE * sizeof(float)));
    fill(a);
    fill(b);
    double cycles_naive = benchmark_rdtsc(add_vectors_naive, a, b, result, VECTOR_SIZE);
    std::cout << "Naive:     " << std::fixed << std::setprecision(0)
              << cycles_naive << " cycles\n";

    double cycles_sse = benchmark_rdtsc(add_vectors_sse, a, b, result, VECTOR_SIZE);
    std::cout << "SSE:       " << std::fixed << std::setprecision(0)
              << cycles_sse << " cycles (speedup: "
              << std::setprecision(2) << (cycles_naive / cycles_sse) << "x)\n";

    double cycles_avx = benchmark_rdtsc(add_vectors_avx, a, b, result, VECTOR_SIZE);
    std::cout << "AVX:       " << std::fixed << std::setprecision(0)
              << cycles_avx << " cycles (speedup: "
              << std::setprecision(2) << (cycles_naive / cycles_avx) << "x)\n";

    std::cout << "\n";


    volatile float sink = 0;
    for (size_t i = 0; i < VECTOR_SIZE; i += 1000) {
        sink += result[i];
    }

    free(a);
    free(b);
    free(result);

    return 0;
}
