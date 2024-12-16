#include <cstdlib>
#pragma GCC target("avx2")

#include <benchmark/benchmark.h>
#include <immintrin.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

static const uint32_t FNV_PRIME = 0x01000193; //   16777619

uint32_t fnv_hash(const int32_t* data, int32_t bytes, uint32_t hash) {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);

    while (bytes--) {
        hash = (*ptr ^ hash) * FNV_PRIME;
        ++ptr;
    }

    return hash;
}

uint32_t fnv_1a_hash(const int32_t* data, int32_t bytes, uint32_t hash) {
    unsigned char* bp = (unsigned char*)data; /* start of buffer */
    unsigned char* be = bp + bytes;           /* beyond end of buffer */

    while (bp < be) {
        /* multiply by the 32 bit FNV magic prime mod 2^32 */
        hash *= FNV_PRIME;
        /* xor the bottom with the current octet */
        hash ^= (int)*bp++;
    }
    /* return our new hash value */
    return hash;
}

void batch_call_fnv_hash(int* data, int hash, int size) {
    for (int i = 0; i < size; ++i) {
        data[i] = fnv_hash(&data[i], 4, hash);
    }
}

void batch_call_fnv_1a_hash(int* data, int hash, int size) {
    for (int i = 0; i < size; ++i) {
        data[i] = fnv_1a_hash(&data[i], 4, hash);
    }
}

void simd_batch_call_fnv_hash(int* data, int hash_base, int size) {
    __m256i fnv_prime_avx = _mm256_set1_epi32(FNV_PRIME);
    __m256i mask = _mm256_set1_epi32(0xFF);
    for (int i = 0; i < size / 8; ++i) {
        __m256i load = _mm256_loadu_si256((__m256i*)data);
        __m256i hash = _mm256_set1_epi32(hash_base);
        for (int j = 0; j < 4; ++j) {
            __m256i data = _mm256_and_si256(load, mask);
            hash = _mm256_mullo_epi32(_mm256_xor_si256(data, hash), fnv_prime_avx);
            load = _mm256_srli_epi32(load, 8);
        }
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(data), hash);
        data += 8;
    }
}

void simd_batch_call_fnv_1a_32_hash(int* data, int hash_base, int size) {
    __m256i fnv_prime_avx = _mm256_set1_epi32(FNV_PRIME);
    for (int i = 0; i < size / 8; ++i) {
        __m256i load = _mm256_loadu_si256((__m256i*)data);
        __m256i hash = _mm256_set1_epi32(hash_base);
        hash = _mm256_mullo_epi32(_mm256_xor_si256(load, hash), fnv_prime_avx);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(data), hash);
        data += 8;
    }
}

void validate(std::vector<int>& data) {
    auto dup = data;
    auto dup2 = data;
    batch_call_fnv_hash(dup.data(), 0, 4096);
    simd_batch_call_fnv_hash(data.data(), 0, 4096);
    for (int i = 0; i < data.size(); ++i) {
        if (dup[i] != data[i]) {
            std::abort();
        }
    }
}

void validate_fnv_1a_32(std::vector<int>& data) {
    auto dup = data;
    auto dup2 = data;
    batch_call_fnv_1a_hash(dup.data(), 0, 4096);
    simd_batch_call_fnv_1a_32_hash(data.data(), 0, 4096);
    for (int i = 0; i < data.size(); ++i) {
        if (dup[i] != data[i]) {
            std::abort();
        }
    }
}


constexpr int elements_sz = 4096;

struct Context {
    std::vector<int32_t> input;
};

Context initCtx() {
    Context ctx;
    ctx.input.resize(elements_sz);

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, elements_sz);

    std::iota(ctx.input.begin(), ctx.input.end(), 0);
    std::random_shuffle(ctx.input.begin(), ctx.input.end());
    return ctx;
}

static void FNV_Impl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        batch_call_fnv_hash(ctx.input.data(), 0, 4096);
        benchmark::DoNotOptimize(ctx);
    }
}

static void FNV_1A_32_Impl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        batch_call_fnv_1a_hash(ctx.input.data(), 0, 4096);
        benchmark::DoNotOptimize(ctx);
    }
}

static void FNV_1A_32_SIMD_Impl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        simd_batch_call_fnv_1a_32_hash(ctx.input.data(), 0, 4096);
        benchmark::DoNotOptimize(ctx);
    }
}

static void FNV_SIMDImpl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        simd_batch_call_fnv_hash(ctx.input.data(), 0, 4096);
        benchmark::DoNotOptimize(ctx);
    }
}

static void validate_fnv(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        validate(ctx.input);
        benchmark::DoNotOptimize(ctx);
    }
}

static void validate_fnv_1a_32(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        validate(ctx.input);
        benchmark::DoNotOptimize(ctx);
    }
}


BENCHMARK(FNV_Impl);
BENCHMARK(FNV_1A_32_Impl);
BENCHMARK(FNV_SIMDImpl);
BENCHMARK(FNV_1A_32_SIMD_Impl);
BENCHMARK(validate_fnv);
BENCHMARK(validate_fnv_1a_32);
BENCHMARK_MAIN();

// --------------------------------------------------------------
// Benchmark                    Time             CPU   Iterations
// --------------------------------------------------------------
// FNV_Impl                 20510 ns        20510 ns        34126
// FNV_1A_32_Impl           12378 ns        12377 ns        56066
// FNV_SIMDImpl              2163 ns         2163 ns       333651
// FNV_1A_32_SIMD_Impl        357 ns          357 ns      1846299
// validate_fnv             27693 ns        27691 ns        25227
// validate_fnv_1a_32       27621 ns        27619 ns        25316