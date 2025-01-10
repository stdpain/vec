#include <benchmark/benchmark.h>
#include <immintrin.h>

#include <algorithm>
#include <cstring>
#include <numeric>
#include <random>
#include <vector>

// result in field [0,255]
constexpr int bucket_sz = 256;
constexpr int elements_sz = 4096;

struct Context {
    std::vector<uint32_t> result;
    std::vector<int> input;
};

Context initCtx() {
    Context ctx;
    ctx.input.resize(elements_sz);
    ctx.result.resize(bucket_sz);

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, bucket_sz);
    for (auto& v : ctx.result) {
        v = distribution(generator);
    }
    return ctx;
}

static void NormalImpl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        ctx.result.assign(bucket_sz, 0);
        for (uint32_t i = 0; i < elements_sz; ++i) {
            ctx.result[ctx.input[i]]++;
        }
        benchmark::DoNotOptimize(ctx);
    }
}

// ref: https://stackoverflow.com/questions/39266476/how-to-speed-up-this-histogram-of-lut-lookups
static void NormalNoDeps(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        ctx.result.assign(bucket_sz, 0);
        alignas(16) uint16_t tmpbins[bucket_sz][4] = {{0}};
        for (uint32_t i = 0; i + 4 <= elements_sz; i += 4) {
            tmpbins[ctx.input[i + 0]][0]++;
            tmpbins[ctx.input[i + 1]][1]++;
            tmpbins[ctx.input[i + 2]][2]++;
            tmpbins[ctx.input[i + 3]][3]++;
        }
        for (uint32_t i = 0; i < bucket_sz; ++i) {
            ctx.result[i] = tmpbins[i][0] + tmpbins[i][1] + tmpbins[i][2] + tmpbins[i][3];
        }
        benchmark::DoNotOptimize(ctx);
    }
}

// https://stackoverflow.com/questions/61122144/micro-optimization-of-a-4-bucket-histogram-of-a-large-array-or-list
static void Histogram4(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        ctx.result.assign(bucket_sz, 0);
        uint32_t counts[4] = {0, 0, 0, 0};
        const int* __restrict arr = ctx.input.data();
        for (uint32_t i = 0; i <= elements_sz; i++) {
            counts[0] += arr[i] == 0;
            counts[1] += arr[i] == 1;
            counts[2] += arr[i] == 2;
            counts[3] += arr[i] == 3;
        }
        for (size_t i = 0; i < 4; ++i) {
            ctx.result[i] = counts[i];
        }

        benchmark::DoNotOptimize(ctx);
    }
}

template <int N>
static void HistogramN(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        ctx.result.assign(bucket_sz, 0);
        uint32_t counts[N];
        memset(counts, 0, sizeof(counts));
        const int* __restrict arr = ctx.input.data();
        for (uint32_t i = 0; i <= elements_sz; i++) {
            // unroll here
            for (int j = 0; j < N; ++j) {
                counts[j] += arr[i] == j;
            }
        }
        for (size_t i = 0; i < N; ++i) {
            ctx.result[i] = counts[i];
        }
        benchmark::DoNotOptimize(ctx);
    }
}

static void GatherImpl(benchmark::State& state) {
    auto ctx = initCtx();
    for (auto _ : state) {
        ctx.result.assign(bucket_sz, 0);
        alignas(16) uint32_t tmpbins[bucket_sz][16] = {{0}};
        for (uint32_t i = 0; i + 16 <= elements_sz; i += 16) {
            __m512i idx = _mm512_loadu_si512(ctx.input.data() + i);
            __m512i values = _mm512_i32gather_epi32(idx, tmpbins, 4);
            values = _mm512_add_epi32(values, _mm512_set1_epi32(1));
            _mm512_i32scatter_epi32(tmpbins, idx, values, 4);
        }
        for (uint32_t i = 0; i < bucket_sz; ++i) {
            for (size_t j = 0; j < 16; ++j) {
                ctx.result[i] += tmpbins[i][j];
            }
        }
        benchmark::DoNotOptimize(ctx);
    }
}

BENCHMARK(NormalImpl);
BENCHMARK(NormalNoDeps);
BENCHMARK(Histogram4);
BENCHMARK_TEMPLATE(HistogramN, 8);
BENCHMARK_TEMPLATE(HistogramN, 32);
BENCHMARK(GatherImpl);

BENCHMARK_MAIN();

// 
// ---------------------------------------------------------
// Benchmark               Time             CPU   Iterations
// ---------------------------------------------------------
// NormalImpl           6878 ns         6878 ns       101835
// NormalNoDeps         2235 ns         2235 ns       313240
// Histogram4            580 ns          580 ns      1185211
// HistogramN<8>        1140 ns         1140 ns       614686
// HistogramN<32>       4532 ns         4532 ns       153859
// GatherImpl           5094 ns         5093 ns       137144