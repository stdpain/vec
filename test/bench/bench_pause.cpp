#include <benchmark/benchmark.h>
#include <immintrin.h>

#include <thread>

static void bench_pause(benchmark::State& state) {
    for (auto _ : state) {
        _mm_pause();
    }
}

static void bench_asm_noop(benchmark::State& state) {
    for (auto _ : state) {
        __asm__ __volatile__("nop");
    }
}
static void bench_noop(benchmark::State& state) {
    for (auto _ : state) {
    }
}

static void bench_thread_yield(benchmark::State& state) {
    for (auto _ : state) {
        std::this_thread::yield();
    }
}

BENCHMARK(bench_noop);
BENCHMARK(bench_asm_noop);
BENCHMARK(bench_pause);
BENCHMARK(bench_thread_yield);
BENCHMARK_MAIN();
