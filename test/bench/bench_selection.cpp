#include <benchmark/benchmark.h>
#include <emmintrin.h>
#include <immintrin.h>

#include <tuple>

void scalar_selection(size_t count, const uint8_t* nulls, const uint32_t* src_data,
                      uint32_t* dst_data) {
    size_t cnt = 0;
    for (size_t i = 0; i < count; ++i) {
        dst_data[i] = src_data[cnt];
        cnt += !nulls[i];
    }
    // for (size_t i = 0; i < count; ++i) {
    //     if (!nulls[i]) {
    //         dst_data[i] = src_data[cnt++];
    //     }
    // }
}

void scalar_selection2(size_t count, const uint8_t* nulls, const uint32_t* src_data,
                       uint32_t* dst_data) {
    size_t cnt = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!nulls[i]) {
            dst_data[i] = src_data[cnt++];
        }
    }
}

void avx512_selection(size_t count, const uint8_t* nulls, const uint32_t* src_data,
                      uint32_t* dst_data) {
    size_t cnt = 0;
    size_t i = 0;
    for (; i + 16 <= count; i += 16) {
        __mmask16 null_mask;
        __m128i mask = _mm_loadu_epi8(&nulls[i]);
        null_mask = _mm_cmp_epi8_mask(mask, _mm_setzero_si128(), _MM_CMPINT_EQ);
        __m512i loaded = _mm512_maskz_expandloadu_epi32(null_mask, &src_data[cnt]);
        cnt += _mm_popcnt_u32(null_mask);
        _mm512_storeu_epi32(&dst_data[i], loaded);
    }
    for (; i < count; ++i) {
        dst_data[i] = src_data[cnt];
        cnt += !nulls[i];
    }
}

struct Bench {
    Bench(size_t size) {
        count = size;
        nulls.resize(count);
        srcs.resize(count);
        dsts.resize(count);
    }

    auto args() { return std::make_tuple(count, nulls.data(), srcs.data(), dsts.data()); }

    size_t count;
    std::vector<uint8_t> nulls;
    std::vector<uint32_t> srcs;
    std::vector<uint32_t> dsts;
};

static void Selection(benchmark::State& state) {
    Bench bench(4096);
    auto args = bench.args();

    for (auto _ : state) {
        std::apply(scalar_selection, args);
    }
}

static void Selection2(benchmark::State& state) {
    Bench bench(4096);
    auto args = bench.args();

    for (auto _ : state) {
        std::apply(scalar_selection2, args);
    }
}

static void SIMDSelection(benchmark::State& state) {
    Bench bench(4096);
    auto args = bench.args();

    for (auto _ : state) {
        std::apply(avx512_selection, args);
    }
}

BENCHMARK(Selection);
BENCHMARK(Selection2);
BENCHMARK(SIMDSelection);
BENCHMARK_MAIN();

// Running ./build_Release/test/bench_selection.out
// Run on (104 X 3800 MHz CPU s)
// CPU Caches:
//   L1 Data 32 KiB (x52)
//   L1 Instruction 32 KiB (x52)
//   L2 Unified 1024 KiB (x52)
//   L3 Unified 36608 KiB (x2)
// Load Average: 2.75, 2.96, 2.91
// --------------------------------------------------------
// Benchmark              Time             CPU   Iterations
// --------------------------------------------------------
// Selection           2693 ns         2693 ns       260442
// Selection2          2683 ns         2683 ns       261424
// SIMDSelection        506 ns          506 ns      1387898
