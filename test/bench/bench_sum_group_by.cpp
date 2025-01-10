#include <benchmark/benchmark.h>
#include <immintrin.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

constexpr int bucket_sz = 256;
constexpr int elements_sz = 4096;

struct Context {
    std::vector<int> input;
    std::vector<int> groupbys;
};

Context initCtx() {
    Context ctx;
    ctx.input.resize(elements_sz);
    ctx.groupbys.resize(elements_sz);

    std::default_random_engine generator;
    // [a, b]
    std::uniform_int_distribution<int> distribution(0, bucket_sz - 1);

    // for (auto& v : ctx.groupbys) {
    //     v = distribution(generator);
    // }
    for (size_t i = 0; i < ctx.groupbys.size(); ++i) {
        if (i < 1000) {
            ctx.groupbys[i] = 1;
        } else if (i < 2000) {
            ctx.groupbys[i] = 2;
        } else {
            ctx.groupbys[i] = 3;
        }
        // ctx.groupbys[i] = i / 200;
        // ctx.groupbys[i] = i % 3;
    }

    return ctx;
}

static void NormalImpl(benchmark::State& state) {
    auto ctx = initCtx();
    std::vector<size_t> states;
    states.resize(bucket_sz);
    for (auto _ : state) {
        for (uint32_t i = 0; i < elements_sz; ++i) {
            states[ctx.groupbys[i]] += ctx.input[i];
        }
        benchmark::DoNotOptimize(ctx);
    }
}

static void UnrollImpl(benchmark::State& state) {
    auto ctx = initCtx();
    auto states = std::make_unique<size_t[]>(bucket_sz * 4);

    for (auto _ : state) {
        for (uint32_t i = 0; i + 4 <= elements_sz; i += 4) {
            states[ctx.groupbys[i + 0] * 4 + 0] += ctx.input[i + 0];
            states[ctx.groupbys[i + 1] * 4 + 1] += ctx.input[i + 1];
            states[ctx.groupbys[i + 2] * 4 + 2] += ctx.input[i + 2];
            states[ctx.groupbys[i + 3] * 4 + 3] += ctx.input[i + 3];
        }
        for (uint32_t i = 0; i < bucket_sz; ++i) {
            // clang-format off
            states[i] = states[i * 4 + 0] + states[i * 4 + 1] + states[i * 4 + 2] + states[i * 4 + 3];
            // clang-format on
        }
        benchmark::DoNotOptimize(ctx);
    }
}

BENCHMARK(NormalImpl);
BENCHMARK(UnrollImpl);

BENCHMARK_MAIN();

// -----------------------------------------------------
// Benchmark           Time             CPU   Iterations
// -----------------------------------------------------
// NormalImpl       6354 ns         6343 ns       111093
// UnrollImpl       3941 ns         3940 ns       136769