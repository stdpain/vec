// https://stackoverflow.com/questions/40950254/speed-up-random-memory-access-using-prefetch

#include <benchmark/benchmark.h>
#include <emmintrin.h>
#include <limits.h>

#include <cstdlib>

constexpr int chunk_size = 4096;

struct BenchContext {
    size_t buffer_size = 0;
    unsigned int* valueBuffer = nullptr;
    unsigned int* indexBuffer = nullptr;
    template <void (BenchContext::*index_creator)(int)>
    void init(size_t buffer_size) {
        this->buffer_size = buffer_size;
        valueBuffer = (unsigned int*)malloc(buffer_size * sizeof(unsigned int));
        indexBuffer = (unsigned int*)malloc(buffer_size * sizeof(unsigned int));
        createValueBuffer(buffer_size);
        (this->*index_creator)(buffer_size);
    }
    ~BenchContext() { close(); }

    unsigned int randomUint() {
        int value = rand() % UINT_MAX;
        return value;
    }

    unsigned int* createValueBuffer(int buffer_size) {
        for (unsigned long i = 0; i < buffer_size; i++) {
            valueBuffer[i] = randomUint();
        }

        return (valueBuffer);
    }

    void createLinerIndexBuffer(int buffer_size) {
        for (unsigned long i = 0; i < buffer_size; i++) {
            indexBuffer[i] = i;
        }
    }

    void createRandIndexBuffer(int buffer_size) {
        for (unsigned long i = 0; i < buffer_size; i++) {
            indexBuffer[i] = rand() % buffer_size;
        }
    }

    void close() {
        free(valueBuffer);
        free(indexBuffer);
    }
};

struct Noop {
    void operator()(BenchContext& context, size_t offset) {}
};

struct FlushCache {
    void operator()(BenchContext& context, size_t offset) {
        for (size_t i = offset; i < chunk_size; ++i) {
            unsigned int index = context.indexBuffer[i];
            _mm_clflush(context.indexBuffer + index);
            _mm_clflush(context.valueBuffer + index);
        }
    }
};

__attribute_noinline__ void call_sum(unsigned int* __restrict valueBuffer,
                                     unsigned int* __restrict indexBuffer, int start, int end,
                                     int64_t& sum) {
    for (unsigned int i = start; i < end; i++) {
        unsigned int index = indexBuffer[i];
        sum += valueBuffer[index];
    }
}

int next(int id, size_t buffer_size) {
    if (id + chunk_size < buffer_size) {
        return id + chunk_size;
    }
    return 0;
}

template <size_t buffer_size, class Pre = Noop>
void mem_liner_access(benchmark::State& state) {
    srand(0);
    BenchContext context;
    context.init<&BenchContext::createLinerIndexBuffer>(buffer_size);
    int64_t sum = 0;
    int id = 0;
    for (auto _ : state) {
        state.PauseTiming();
        Pre()(context, id);
        state.ResumeTiming();
        call_sum(context.valueBuffer, context.indexBuffer, id, id + chunk_size, sum);
        id = next(id, buffer_size);
    }
}

template <size_t buffer_size, class Pre = Noop>
void run_rand_access(benchmark::State& state) {
    srand(0);
    BenchContext context;
    context.init<&BenchContext::createRandIndexBuffer>(buffer_size);
    int64_t sum = 0;
    int id = 0;
    for (auto _ : state) {
        state.PauseTiming();
        Pre()(context, id);
        state.ResumeTiming();
        call_sum(context.valueBuffer, context.indexBuffer, id, id + chunk_size, sum);
        id = next(id, buffer_size);
    }
}

BENCHMARK_TEMPLATE(mem_liner_access, 4096 * 10);
BENCHMARK_TEMPLATE(mem_liner_access, 4096 * 250);
BENCHMARK_TEMPLATE(mem_liner_access, 4096 * 1000);
BENCHMARK_TEMPLATE(run_rand_access, 4096 * 10);
BENCHMARK_TEMPLATE(run_rand_access, 4096 * 10, FlushCache);
BENCHMARK_TEMPLATE(run_rand_access, 4096 * 250);
BENCHMARK_TEMPLATE(run_rand_access, 4096 * 1000);
// slow benchmarks...
BENCHMARK_TEMPLATE(mem_liner_access, 4096 * 100000);
BENCHMARK_TEMPLATE(run_rand_access, 4096 * 100000);

BENCHMARK_MAIN();

// benchmark on Intel(R) Xeon(R) Platinum 8269CY CPU @ 2.50GHz (104 CPUS)
// CPU Caches:
//   L1 Data 32 KiB (x52)
//   L1 Instruction 32 KiB (x52)
//   L2 Unified 1024 KiB (x52)
//   L3 Unified 36608 KiB (x2)
// mem_liner_access<4096 * 10>                  2474 ns         2490 ns       281048
// mem_liner_access<4096 * 250>                 2521 ns         2531 ns       275732
// mem_liner_access<4096 * 1000>                2745 ns         2756 ns       247025
// run_rand_access<4096 * 10>                   2497 ns         2511 ns       278192
// run_rand_access<4096 * 10, FlushCache>       5549 ns         5096 ns       137866
// run_rand_access<4096 * 250>                  9067 ns         9086 ns        76910
// run_rand_access<4096 * 1000>                14822 ns        14865 ns        38327
// mem_liner_access<4096 * 100000>              3528 ns         3547 ns       203628
// run_rand_access<4096 * 100000>              59816 ns        59893 ns        11171
