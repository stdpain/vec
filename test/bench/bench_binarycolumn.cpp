
#include <benchmark/benchmark.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

#include "vec/binary_vec.h"

using namespace vec;

const int batch_size = 4096;

struct Generate_SSB_SHIP_MODE {
    Slice operator()() {
        auto& str = ssb_shipmode[rand() % ssb_shipmode.size()];
        return Slice(str.data(), str.size());
    }
    template <class TypeSlice>
    TypeSlice get() {
        auto& str = ssb_shipmode[rand() % ssb_shipmode.size()];
        return TypeSlice(str.data(), str.size());
    }

private:
    inline static std::vector<std::string> ssb_shipmode{"TRUCK",   "AIR",  "RAIL", "MAIL",
                                                        "REG AIR", "SHIP", "FOB"};
};

template <int x>
struct Generate_FIXED_x {
    Generate_FIXED_x() {
        for (int i = 0; i < batch_size; ++i) {
            std::string raw_val;
            for (int j = 0; j < x; ++j) {
                raw_val.push_back(rand() % 256);
            }
            buffers.emplace_back(std::move(raw_val));
        }
    }

    Slice operator()() {
        auto& str = buffers[rand() % buffers.size()];
        return Slice(str.data(), str.size());
    };

    template <class TypeSlice>
    TypeSlice get() {
        auto& str = buffers[rand() % buffers.size()];
        return TypeSlice(str.data(), str.size());
    }

private:
    inline static std::vector<std::string> buffers;
};

struct Generate {
    template <class T>
    static void build(int size, std::vector<Slice>& slices, T& gen) {
        slices.reserve(size);
        for (int i = 0; i < size; ++i) {
            slices.emplace_back(gen());
        }
    }
};

template <class Col, class StringGen>
static void StringColCreateBench(benchmark::State& state) {
    StringGen gen;
    for (auto _ : state) {
        {
            Col vec;
            std::vector<Slice> slices;
            state.PauseTiming();
            Generate::build(batch_size, slices, gen);
            state.ResumeTiming();
            vec.build_strings(slices);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }
}

template <class StringGen>
constexpr void (*FlatBinaryColCreate)(benchmark::State& state) =
        &StringColCreateBench<vec::FlatBinaryVec, StringGen>;

template <class StringGen>
constexpr void (*InlineBinaryColCreate)(benchmark::State& state) =
        &StringColCreateBench<vec::InlineBinaryVec, StringGen>;

template <class Col, class TypeSlice, class StringGen>
static void CompareBench(benchmark::State& state) {
    StringGen gen;
    Col vec;
    std::vector<Slice> slices;
    Generate::build(batch_size, slices, gen);
    vec.build_strings(slices);
    std::vector<uint8_t> filter(batch_size);
    for (auto _ : state) {
        auto slice = gen.template get<TypeSlice>();
        for (int i = 0; i < batch_size; ++i) {
            filter[i] = vec.get_slice(i) == slice;
        }
    }
}

// BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_SSB_SHIP_MODE);
// BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<4>);
// BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<8>);
// BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<16>);
// BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<128>);

// BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_SSB_SHIP_MODE);
// BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<4>);
// BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<8>);
// BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<16>);
// BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<128>);

template <class StringGen>
constexpr void (*InlineBinaryCmpBench)(benchmark::State& state) =
        &CompareBench<vec::InlineBinaryVec, SliceInline, StringGen>;

template <class StringGen>
constexpr void (*FlatBinaryCmpBench)(benchmark::State& state) =
        &CompareBench<vec::FlatBinaryVec, Slice, StringGen>;

BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_SSB_SHIP_MODE);
BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_FIXED_x<4>);
BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_FIXED_x<128>);

BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_SSB_SHIP_MODE);
BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_FIXED_x<4>);
BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_FIXED_x<128>);

BENCHMARK_MAIN();
