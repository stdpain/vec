#pragma GCC target("avx2")
#include <benchmark/benchmark.h>
#include <immintrin.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

constexpr int bucket_sz = 1024;
constexpr int elements_sz = 4096 * 1000;
constexpr int probe_sz = 4096;

struct Context {
    std::vector<uint32_t> result;
    std::vector<uint32_t> input;
    std::vector<uint32_t> buckets;
};

Context initCtx() {
    Context ctx;
    ctx.buckets.resize(probe_sz);
    ctx.input.resize(bucket_sz);
    ctx.result.resize(elements_sz);

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, bucket_sz);

    std::generate(ctx.buckets.begin(), ctx.buckets.end(),
                  [&]() { return distribution(generator); });

    std::iota(ctx.input.begin(), ctx.input.end(), 0);
    std::random_shuffle(ctx.input.begin(), ctx.input.end());
    return ctx;
}

static void NormalImpl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        for (uint32_t i = 0; i < probe_sz; ++i) {
            ctx.result[i] = ctx.input[ctx.buckets[i]];
        }
        benchmark::DoNotOptimize(ctx);
    }
}

static void SIMDImpl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        constexpr int mini_batch = 256 / 32;
        constexpr int loop_sz = probe_sz / mini_batch;
        const uint32_t* data = ctx.buckets.data();
        uint32_t* dst = ctx.result.data();
        for (int i = 0; i < loop_sz; ++i) {
            __m256i loaded = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data));
            __m256i gathered = _mm256_i32gather_epi32((int32_t*)ctx.input.data(), loaded, 4);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst), gathered);
            data += 8;
            dst += 8;
        }
        benchmark::DoNotOptimize(ctx);
    }
}

BENCHMARK(NormalImpl);
BENCHMARK(SIMDImpl);
BENCHMARK_MAIN();
