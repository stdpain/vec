#include <benchmark/benchmark.h>
#include <limits.h>

// constexpr int BUFFER_SIZE = 4096 * 100000;
constexpr int BUFFER_SIZE = 4096 * 250;
constexpr int chunk_size = 4096;
unsigned int* valueBuffer;
unsigned int* indexBuffer;

unsigned int randomUint() {
    int value = rand() % UINT_MAX;
    return value;
}

unsigned int* createValueBuffer() {
    for (unsigned long i = 0; i < BUFFER_SIZE; i++) {
        valueBuffer[i] = randomUint();
    }

    return (valueBuffer);
}

unsigned int* createRandIndexBuffer() {
    for (unsigned long i = 0; i < BUFFER_SIZE; i++) {
        indexBuffer[i] = rand() % BUFFER_SIZE;
    }

    return (indexBuffer);
}

unsigned int* createLinerIndexBuffer() {
    for (unsigned long i = 0; i < BUFFER_SIZE; i++) {
        indexBuffer[i] = i;
    }

    return (indexBuffer);
}

template <unsigned int* index_creator()>
void init() {
    valueBuffer = (unsigned int*)malloc(BUFFER_SIZE * sizeof(unsigned int));
    indexBuffer = (unsigned int*)malloc(BUFFER_SIZE * sizeof(unsigned int));
    index_creator();
}

void close() {
    free(valueBuffer);
    free(indexBuffer);
}

__attribute_noinline__ void call_sum(int start, int end, int64_t& sum) {
    for (unsigned int i = start; i < end; i++) {
        unsigned int index = indexBuffer[i];
        sum += valueBuffer[index];
    }
}

int next(int id) {
    if (id + chunk_size < BUFFER_SIZE) {
        return id + chunk_size;
    }
    return 0;
}

void mem_liner_access(benchmark::State& state) {
    init<createLinerIndexBuffer>();
    int64_t sum = 0;
    int id = 0;
    for (auto _ : state) {
        call_sum(id, id + chunk_size, sum);
        id = next(id);
    }
    close();
}

void run_rand_access(benchmark::State& state) {
    init<createRandIndexBuffer>();
    int64_t sum = 0;
    int id = 0;
    for (auto _ : state) {
        call_sum(id, id + chunk_size, sum);
        id = next(id);
    }
    close();
}
BENCHMARK(run_rand_access);
BENCHMARK(mem_liner_access);

BENCHMARK_MAIN();
