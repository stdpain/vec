#define BENCHMARK_HAS_CXX11
#include <benchmark/benchmark.h>
#include <immintrin.h>

#include <cstring>
#include <vector>

constexpr int chunk_size = 4096;

using Filter = std::vector<uint8_t>;
using Container = std::vector<int32_t>;

template <class T>
struct AlwaysZeroGenerator {
    static T next_data() { return 0; }
};

template <class T>
struct AlwaysOneGenerator {
    static T next_data() { return 1; }
};

struct RandomGenerator {
    static uint8_t next_data() { return rand() % 2; }
};

template <class DataGenerator>
struct FilterIniter {
    static void filter_init(Filter& filter) {
        filter.resize(chunk_size);
        for (int i = 0; i < filter.size(); ++i) {
            filter[i] = DataGenerator::next_data();
        }
    }
};

inline int CountTrailingZeros32(uint32_t n) {
    return n == 0 ? 32 : __builtin_ctz(n);
}

using CurrentFilterIniter = FilterIniter<RandomGenerator>;
// using CurrentFilterIniter = FilterIniter<AlwaysOneGenerator<uint8_t>>;
// using CurrentFilterIniter = FilterIniter<AlwaysZeroGenerator<uint8_t>>;

int NormalAVXFilter(Filter& filter, Container& data) {
    size_t num_rows = data.size();
    const uint8_t* filt_pos = filter.data();
    const uint8_t* filt_end = filt_pos + num_rows;
    const auto* data_pos = data.data();
    int res_index = 0;

    static constexpr size_t SIMD_BYTES = 32;
    static constexpr size_t TYPE_SIZE = 4;
    constexpr size_t kBatchNums = 256 / (8 * sizeof(uint8_t));

    const __m256i zero16 = _mm256_setzero_si256();
    const uint8_t* filt_end_avx = filt_pos + num_rows / SIMD_BYTES * SIMD_BYTES;

    while (filt_pos < filt_end_avx) {
        uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(
                _mm256_loadu_si256(reinterpret_cast<const __m256i*>(filt_pos)), zero16));

        if (mask == 0) {
        } else if (0xFFFFFFFF == mask) {
            memmove(data.data() + res_index, data_pos, TYPE_SIZE * 4);
            res_index += 4;
        } else {
            while (mask) {
                size_t index = __builtin_ctzll(mask);
                data[res_index++] = data_pos[index];
                mask = mask & (mask - 1);
            }
        }

        filt_pos += SIMD_BYTES;
        data_pos += SIMD_BYTES;
    }

    while (filt_pos < filt_end) {
        if (*filt_pos) data[res_index] = *data_pos;

        ++filt_pos;
        ++data_pos;
    }

    return res_index;
}

template <typename Initer, int FILTER(Filter& filter, Container& data)>
void BENCH_Filter(benchmark::State& state) {
    Container container;
    container.resize(chunk_size);
    Filter filter;
    Initer::filter_init(filter);
    for (auto _ : state) {
        FILTER(filter, container);
    }
}

int NormalSSEFilter(Filter& filter, Container& data) {
    size_t num_rows = data.size();

    const uint8_t* filt_pos = filter.data();
    const uint8_t* filt_end = filt_pos + num_rows;
    const auto* data_pos = data.data();
    int res_index = 0;

    static constexpr size_t SIMD_BYTES = 16;
    static constexpr size_t TYPE_SIZE = 4;

    const __m128i zero16 = _mm_setzero_si128();
    const uint8_t* filt_end_sse = filt_pos + num_rows / SIMD_BYTES * SIMD_BYTES;

    while (filt_pos < filt_end_sse) {
        uint16_t mask = _mm_movemask_epi8(_mm_cmpgt_epi8(
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(filt_pos)), zero16));

        if (0xFFFF == mask) {
            memmove(data.data() + res_index, data_pos, TYPE_SIZE * 4);
            res_index += 4;
        } else {
            while (mask) {
                size_t index = __builtin_ctz(mask);
                data[res_index++] = data_pos[index];
                mask = mask & (mask - 1);
            }
        }

        filt_pos += SIMD_BYTES;
        data_pos += SIMD_BYTES;
    }

    while (filt_pos < filt_end) {
        if (*filt_pos) data[res_index] = *data_pos;

        ++filt_pos;
        ++data_pos;
    }
    return res_index;
}

int AVXBranchLess(Filter& filter, Container& container) {
    auto* f = filter.data();
    auto* r = container.data();
    int n = container.size();
    assert(n % 8 == 0);
    int* output = r;
    for (int i = 0; i < n; i += 8) {
        // load data
        int ld_filter = *(int64_t*)(f + i);
        __m128i v = _mm_set1_epi64x(ld_filter);
        __m256i loaded_mask = _mm256_cvtepi8_epi32(v);
        __m256i mask = _mm256_cmpgt_epi8(loaded_mask, _mm256_setzero_si256());

        __m256 ld = _mm256_loadu_ps((float*)(r + i));

        //https://stackoverflow.com/questions/36932240/avx2-what-is-the-most-efficient-way-to-pack-left-based-on-a-mask#comment61425284_36932240
        // _mm256_movemask_epi8
        // int move_mask = _mm256_movemask_ps(mask);                               // if we need index at 0, 2
        int move_mask = _mm256_movemask_epi8(mask); // if we need index at 0, 2
                                                    // move_mask will be 0b 0000'0101
        {                                           // store filtered data
            uint64_t expanded_mask =
                    _pdep_u64(move_mask, 0x0101010101010101); // unpack each bit to a byte
            // output will be  0b 0000'00001| 0000'0000 | 0000'00001

            expanded_mask *= 0xFF; // mask |= mask<<1 | mask<<2 | ... | mask<<7;
                    // 0b ABC... -> 0b AAAA'AAAA'BBBB'BBBB'CCCC'CCCC...: replicate each bit to fill its byte

            const uint64_t identity_indices =
                    0x0706050403020100; // the identity shuffle for vpermps, packed to one index per byte
            uint64_t wanted_indices =
                    _pext_u64(identity_indices, expanded_mask); // eg. we want index 0 2
            // expanded_mask will be   0b000'...'1111 1111'0000 0000'1111 1111
            //     [....,1,0,1]
            // wanted_indices will be  0b000'...'0000 0010'0000 0000
            //     [......,2,0]

            __m128i bytevec = _mm_cvtsi64_si128(
                    wanted_indices); // add zeros at the beginning of wanted_indices, and it becomes __m128i
                                     // just a cast

            __m256i shufmask = _mm256_cvtepu8_epi32(bytevec); // extand 8 packed 8bit int to 32bit
                                                              // abc -> 000a 000b 000c

            __m256 newdata =
                    _mm256_permutevar8x32_ps(ld, shufmask); // shuffle unfiltered data across lane

            _mm256_storeu_ps((float*)output, newdata); // store filtered data
        }
        // MOVE pointer
        output += _mm_popcnt_u64(move_mask);
    }

    return (int)(output - r);
}

int ScalarBranceLess(Filter& filter, Container& container) {
    auto* f = filter.data();
    int n = container.size();
    int cnt = 0;
    for (size_t i = 0; i < n; ++i) {
        container[cnt] = container[i];
        cnt += f[i] != 0;
    }
    return cnt;
}

int ScalarFilter(Filter& filter, Container& container) {
    auto* f = filter.data();
    int n = container.size();
    int cnt = 0;
    for (size_t i = 0; i < n; ++i) {
        if (f[i]) {
            container[cnt++] = container[i];
        }
    }
    return cnt;
}

BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<RandomGenerator>, NormalAVXFilter);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<RandomGenerator>, AVXBranchLess);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<RandomGenerator>, NormalSSEFilter);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<RandomGenerator>, ScalarBranceLess);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<RandomGenerator>, ScalarFilter);

BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysOneGenerator<uint8_t>>, NormalAVXFilter);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysOneGenerator<uint8_t>>, AVXBranchLess);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysOneGenerator<uint8_t>>, NormalSSEFilter);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysOneGenerator<uint8_t>>, ScalarBranceLess);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysOneGenerator<uint8_t>>, ScalarFilter);

BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysZeroGenerator<uint8_t>>, NormalAVXFilter);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysZeroGenerator<uint8_t>>, AVXBranchLess);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysZeroGenerator<uint8_t>>, NormalSSEFilter);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysZeroGenerator<uint8_t>>, ScalarBranceLess);
BENCHMARK_TEMPLATE(BENCH_Filter, FilterIniter<AlwaysZeroGenerator<uint8_t>>, ScalarFilter);

BENCHMARK_MAIN();


// ---------------------------------------------------------------------------------------------------------------------
// Benchmark                                                                           Time             CPU   Iterations
// ---------------------------------------------------------------------------------------------------------------------
// BENCH_Filter<FilterIniter<RandomGenerator>, NormalAVXFilter>                     2226 ns         2226 ns       311415
// BENCH_Filter<FilterIniter<RandomGenerator>, AVXBranchLess>                       1158 ns         1158 ns       607556
// BENCH_Filter<FilterIniter<RandomGenerator>, NormalSSEFilter>                     2182 ns         2181 ns       301329
// BENCH_Filter<FilterIniter<RandomGenerator>, ScalarBranceLess>                    2619 ns         2618 ns       270948
// BENCH_Filter<FilterIniter<RandomGenerator>, ScalarFilter>                       12922 ns        12916 ns        54577
// BENCH_Filter<FilterIniter<AlwaysOneGenerator<uint8_t>>, NormalAVXFilter>          131 ns          131 ns      5423553
// BENCH_Filter<FilterIniter<AlwaysOneGenerator<uint8_t>>, AVXBranchLess>           1149 ns         1149 ns       606757
// BENCH_Filter<FilterIniter<AlwaysOneGenerator<uint8_t>>, NormalSSEFilter>          330 ns          330 ns      2123937
// BENCH_Filter<FilterIniter<AlwaysOneGenerator<uint8_t>>, ScalarBranceLess>        2733 ns         2732 ns       256189
// BENCH_Filter<FilterIniter<AlwaysOneGenerator<uint8_t>>, ScalarFilter>            3006 ns         3005 ns       232957
// BENCH_Filter<FilterIniter<AlwaysZeroGenerator<uint8_t>>, NormalAVXFilter>         170 ns          170 ns      4123567
// BENCH_Filter<FilterIniter<AlwaysZeroGenerator<uint8_t>>, AVXBranchLess>          1401 ns         1401 ns       564596
// BENCH_Filter<FilterIniter<AlwaysZeroGenerator<uint8_t>>, NormalSSEFilter>         280 ns          280 ns      2788675
// BENCH_Filter<FilterIniter<AlwaysZeroGenerator<uint8_t>>, ScalarBranceLess>       2607 ns         2607 ns       259076
// BENCH_Filter<FilterIniter<AlwaysZeroGenerator<uint8_t>>, ScalarFilter>           2943 ns         2942 ns       262539