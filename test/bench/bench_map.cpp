#include <benchmark/benchmark.h>
#include <phmap.h>
#include <tsl/robin_map.h>

#include <iostream>
#include <random>
#include <unordered_map>

constexpr int data_range = 5'000'000;
size_t chunk_size = 4096;
static std::default_random_engine e;
static std::uniform_int_distribution<int> u(0, data_range);

template <class MAP, bool prepare_work = true>
void insert(benchmark::State& state) {
    MAP raw_map;
    std::vector<int64_t> input_column(chunk_size);
    // prepare work
    // prepare a large hash table
    if constexpr (prepare_work) {
        for (size_t i = 0; i < data_range * 2; ++i) {
            raw_map.try_emplace(u(e), nullptr);
        }
    }

    for (auto _ : state) {
        state.PauseTiming();
        {
            for (size_t i = 0; i < chunk_size; ++i) {
                input_column[i] = u(e);
            }
        }
        state.ResumeTiming();
        for (size_t i = 0; i < chunk_size; ++i) {
            raw_map.try_emplace(input_column[i], nullptr);
        }
    }
    std::cout << "map size:" << raw_map.size() << std::endl;
}

static void std_insert_test(benchmark::State& state) {
    insert<std::unordered_map<int64_t, char*>>(state);
}

static void flat_hash_map_insert_test(benchmark::State& state) {
    insert<phmap::flat_hash_map<int64_t, char*>>(state);
}

static void robin_hash_map_insert_test(benchmark::State& state) {
    insert<tsl::robin_map<int64_t, char*>>(state);
}

BENCHMARK(std_insert_test);
BENCHMARK(flat_hash_map_insert_test);
BENCHMARK(robin_hash_map_insert_test);

BENCHMARK_MAIN();