
#include <benchmark/benchmark.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

#include "vec/binary_vec.h"

using namespace vec;

const int batch_size = 4096;

struct StringGenerater {
    Slice operator()() {
        auto& str = buffers[rand() % buffers.size()];
        return Slice(str.data(), str.size());
    };

    template <class TypeSlice>
    TypeSlice get() {
        auto& str = buffers[rand() % buffers.size()];
        return TypeSlice(str.data(), str.size());
    }

protected:
    std::vector<std::string> buffers;
};

struct Generate_SSB_SHIP_MODE : StringGenerater {
    Generate_SSB_SHIP_MODE() {
        buffers = {"TRUCK", "AIR", "RAIL", "MAIL", "REG AIR", "SHIP", "FOB"};
    }
};

template <int x, bool same = false>
struct Generate_FIXED_x : StringGenerater {
    Generate_FIXED_x() {
        for (int i = 0; i < batch_size; ++i) {
            std::string raw_val;
            for (int j = 0; j < x; ++j) {
                if constexpr (same) {
                    raw_val.push_back('x');
                } else {
                    raw_val.push_back(rand() % 256);
                }
            }
            buffers.emplace_back(std::move(raw_val));
        }
    }
};

// 2020-03-02
struct Generate_Date_Str : StringGenerater {
public:
    Generate_Date_Str() {
        std::string raw_val = "2020-03-";
        for (int i = 0; i < batch_size; ++i) {
            buffers.emplace_back(raw_val + std::to_string(i % 31));
        }
    }
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
    std::vector<Slice> slices;
    Generate::build(batch_size, slices, gen);

    for (auto _ : state) {
        {
            Col vec;
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

BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_SSB_SHIP_MODE);
BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_Date_Str);
BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<4>);
BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<12>);
BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<12, true>);
BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<16, true>);
BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<128>);
BENCHMARK_TEMPLATE(FlatBinaryColCreate, Generate_FIXED_x<128, true>);

BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_SSB_SHIP_MODE);
BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_Date_Str);
BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<4>);
BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<12>);
BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<12, true>);
BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<16, true>);
BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<128>);
BENCHMARK_TEMPLATE(InlineBinaryColCreate, Generate_FIXED_x<128, true>);

template <class StringGen>
constexpr void (*InlineBinaryCmpBench)(benchmark::State& state) =
        &CompareBench<vec::InlineBinaryVec, SliceInline, StringGen>;

template <class StringGen>
constexpr void (*FlatBinaryCmpBench)(benchmark::State& state) =
        &CompareBench<vec::FlatBinaryVec, Slice, StringGen>;

BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_SSB_SHIP_MODE);
BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_Date_Str);
BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_FIXED_x<4>);
BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_FIXED_x<12>);
BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_FIXED_x<12, true>);
BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_FIXED_x<16, true>);
BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_FIXED_x<128>);
BENCHMARK_TEMPLATE(FlatBinaryCmpBench, Generate_FIXED_x<128, true>);

BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_SSB_SHIP_MODE);
BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_Date_Str);
BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_FIXED_x<4>);
BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_FIXED_x<12>);
BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_FIXED_x<12, true>);
BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_FIXED_x<16, true>);
BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_FIXED_x<128>);
BENCHMARK_TEMPLATE(InlineBinaryCmpBench, Generate_FIXED_x<128, true>);

BENCHMARK_MAIN();
