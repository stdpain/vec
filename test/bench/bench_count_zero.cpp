#include <benchmark/benchmark.h>
#include <emmintrin.h>
#include <immintrin.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <type_traits>
#include <vector>

size_t sse_roll_count_zero(const uint8_t* data, size_t size) {
    int count = 0;
    const uint8_t* end = data + size;

    // __SSE2__ && __POPCNT__
    const __m128i zero16 = _mm_setzero_si128();
    const uint8_t* end64 = data + (size / 64 * 64);

    for (; data < end64; data += 64) {
        count += __builtin_popcountll(
                static_cast<uint64_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(
                        _mm_loadu_si128(reinterpret_cast<const __m128i*>(data)), zero16))) |
                (static_cast<uint64_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(
                         _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 16)), zero16)))
                 << 16u) |
                (static_cast<uint64_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(
                         _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 32)), zero16)))
                 << 32u) |
                (static_cast<uint64_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(
                         _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + 48)), zero16)))
                 << 48u));
    }

    for (; data < end; ++data) {
        count += (*data == 0);
    }
    return count;
}

size_t sse_count_zero(const uint8_t* data, size_t size) {
    const __m128i zero = _mm_setzero_si128();
    const uint8_t* end = data + size;

    int count = 0;
    for (; data + 16 < end; data += 16) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));
        __m128i mask = _mm_cmpeq_epi8(chunk, zero);
        count += _mm_popcnt_u32(_mm_movemask_epi8(mask));
    }

    for (; data < end; ++data) {
        count += (*data == 0);
    }
    return count;
}

int avx_count_zero(const uint8_t* data, size_t size) {
    const __m256i zero = _mm256_setzero_si256();
    const uint8_t* end = data + size;

    int count = 0;
    for (; data + 32 < end; data += 32) {
        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data));
        __m256i mask = _mm256_cmpeq_epi8(chunk, zero);
        count += _mm_popcnt_u32(_mm256_movemask_epi8(mask));
    }

    for (; data < end; ++data) {
        count += (*data == 0);
    }
    return count;
}

constexpr int bucket_sz = 1024;

struct Context {
    std::vector<uint8_t> input;
};

Context initCtx() {
    Context ctx;
    ctx.input.resize(bucket_sz);

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, bucket_sz);

    std::iota(ctx.input.begin(), ctx.input.end(), 0);
    std::random_shuffle(ctx.input.begin(), ctx.input.end());
    return ctx;
}
static void SSEImpl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        int val = sse_roll_count_zero(ctx.input.data(), ctx.input.size());
        benchmark::DoNotOptimize(val);
    }
}

static void SSERollImpl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        int val = sse_roll_count_zero(ctx.input.data(), ctx.input.size());
        benchmark::DoNotOptimize(val);
    }
}



static void AVXImpl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        int val = avx_count_zero(ctx.input.data(), ctx.input.size());
        benchmark::DoNotOptimize(val);
    }
}

BENCHMARK(SSEImpl);
BENCHMARK(SSERollImpl);
BENCHMARK(AVXImpl);
BENCHMARK_MAIN();


// Intel(R) Xeon(R) Platinum 8269CY CPU @ 2.50GHz
// ------------------------------------------------------
// Benchmark            Time             CPU   Iterations
// ------------------------------------------------------
// SSEImpl           28.9 ns         28.9 ns     24174511
// SSERollImpl       28.9 ns         28.9 ns     24344092
// AVXImpl           24.7 ns         24.7 ns     28438163