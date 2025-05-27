#include <benchmark/benchmark.h>
#include <malloc.h>
#include <atomic>

void* static_call();
extern void* plt_call();

static void StaticLink(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(static_call());
    }
}

static void DynamicLink(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(plt_call());
    }
}

void* static_call() {
    static std::atomic_int v{};
    v++;
    return nullptr;
}

BENCHMARK(StaticLink);
BENCHMARK(DynamicLink);
BENCHMARK_MAIN();
