#include <benchmark/benchmark.h>
#include <byteswap.h>
#include <emmintrin.h>
#include <immintrin.h>

#include <algorithm>
#include <cstring>
#include <numeric>
#include <random>
#include <string_view>
#include <vector>

inline unsigned __int128 gbswap_128(unsigned __int128 host_int) {
    return static_cast<unsigned __int128>(bswap_64(static_cast<uint64_t>(host_int >> 64))) |
           (static_cast<unsigned __int128>(bswap_64(static_cast<uint64_t>(host_int))) << 64);
}

// convert BINSZ to int128 dst
template <int binsz>
constexpr __m128i generate_shuf_msk() {
    static_assert(binsz >= 1 && binsz <= 16);
    union {
        __m128i i128;
        uint8_t i8s[sizeof(__m128i)];
    } mask;

    constexpr int32_t length = sizeof(__m128i);
    int32_t seq = 0;
    for (int i = binsz - 1; i >= 0; --i) {
        mask.i8s[seq++] = i;
    }
    for (int i = seq; i < length; ++i) {
        mask.i8s[i] = 0xFF;
    }

    return mask.i128;
}

template <int BINSZ>
void sse_convert(const uint8_t* __restrict src_null_data, uint8_t* __restrict dst_data,
                 size_t length) {
    __m128i mask = generate_shuf_msk<BINSZ>();
    for (size_t i = 0; i < length; ++i) {
        __m128i xmm = _mm_loadu_si128((__m128i*)src_null_data);
        xmm = _mm_shuffle_epi8(xmm, mask);
        _mm_storeu_si128((__m128i*)dst_data, xmm);
        src_null_data += BINSZ;
        dst_data += 16;
    }
}

template <int BINSZ>
void scalar_convert(const uint8_t* __restrict src_null_data, uint8_t* __restrict dst_data,
                    size_t length) {
    for (size_t i = 0; i < length; ++i) {
        __int128_t value;
        memcpy(&value, src_null_data, sizeof(__int128_t));
        value = gbswap_128(value);
        value = value >> ((sizeof(value) - BINSZ) * 8);
        memcpy(dst_data, &value, sizeof(__int128_t));
        src_null_data += BINSZ;
        dst_data += 16;
    }
}

static void SSEImpl(benchmark::State& state) {
    std::vector<uint8_t> srcs;
    std::vector<uint8_t> dsts;

    srcs.resize(12 * 4096 + 15);
    dsts.resize(16 * 4096);

    for (auto _ : state) {
        sse_convert<12>(srcs.data(), dsts.data(), 4096);
    }
}

static void ScalarImpl(benchmark::State& state) {
    std::vector<uint8_t> srcs;
    std::vector<uint8_t> dsts;

    srcs.resize(12 * 4096 + 15);
    dsts.resize(16 * 4096);

    for (auto _ : state) {
        scalar_convert<12>(srcs.data(), dsts.data(), 4096);
    }
}

BENCHMARK(ScalarImpl);
BENCHMARK(SSEImpl);
BENCHMARK_MAIN();

// -----------------------------------------------------
// Benchmark           Time             CPU   Iterations
// -----------------------------------------------------
// ScalarImpl       4603 ns         4603 ns       152183
// SSEImpl          2316 ns         2315 ns       302468