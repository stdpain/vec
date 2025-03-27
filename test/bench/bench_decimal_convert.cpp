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
void sse_convert(const uint8_t* __restrict src_data, uint8_t* __restrict dst_data, size_t length) {
    __m128i mask = generate_shuf_msk<BINSZ>();
    #pragma GCC unroll 2
    for (size_t i = 0; i < length; ++i) {
        __m128i xmm = _mm_loadu_si128((__m128i*)src_data);
        xmm = _mm_shuffle_epi8(xmm, mask);
        _mm_storeu_si128((__m128i*)dst_data, xmm);
        src_data += BINSZ;
        dst_data += 16;
    }
}

template <int BINSZ>
void sse_convert_nullable(const uint8_t* __restrict src_data,
                          const uint8_t* __restrict src_null_data, uint8_t* __restrict dst_data,
                          size_t length) {
    __m128i mask = generate_shuf_msk<BINSZ>();
    size_t i = 0;
    for (i = 0; i + 2 <= length; i += 2) {
        short nulls;
        memcpy(&nulls, src_null_data, 2);
        src_null_data += 2;
        if (nulls == 0x0101) {
        } else if (nulls == 0) {
            __m128i xmm = _mm_lddqu_si128((__m128i*)src_data);
            xmm = _mm_shuffle_epi8(xmm, mask);
            _mm_storeu_si128((__m128i*)dst_data, xmm);

            xmm = _mm_lddqu_si128((__m128i*)(src_data + 12));
            xmm = _mm_shuffle_epi8(xmm, mask);
            _mm_storeu_si128((__m128i*)(dst_data + 16), xmm);
        } else {
            __m128i xmm = _mm_lddqu_si128((__m128i*)src_data);
            xmm = _mm_shuffle_epi8(xmm, mask);
            _mm_storeu_si128((__m128i*)dst_data, xmm);
            src_data += BINSZ * !(nulls & 0x01);

            xmm = _mm_lddqu_si128((__m128i*)src_data);
            xmm = _mm_shuffle_epi8(xmm, mask);
            _mm_storeu_si128((__m128i*)(dst_data + 16), xmm);
            src_data += BINSZ * !(nulls & 0x0100);
        }
        dst_data += sizeof(__m128i) * 2;
    }
    // process tail
    for (; i < length; ++i) {
        __m128i xmm = _mm_loadu_si128((__m128i*)src_data);
        xmm = _mm_shuffle_epi8(xmm, mask);
        _mm_storeu_si128((__m128i*)dst_data, xmm);
        src_data += BINSZ;
        dst_data += sizeof(__m128i);
    }
}

template <int BINSZ>
void scalar_convert(const uint8_t* __restrict src_data, uint8_t* __restrict dst_data,
                    size_t length) {
    for (size_t i = 0; i < length; ++i) {
        __int128_t value;
        memcpy(&value, src_data, sizeof(__int128_t));
        value = gbswap_128(value);
        value = value >> ((sizeof(value) - BINSZ) * 8);
        memcpy(dst_data, &value, sizeof(__int128_t));
        src_data += BINSZ;
        dst_data += 16;
    }
}

template <int BINSZ>
void scalar_nullable_convert(const uint8_t* __restrict src_data,
                             const uint8_t* __restrict src_null_data, uint8_t* __restrict dst_data,
                             size_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (src_null_data[i]) continue;
        __int128_t value;
        memcpy(&value, src_data, sizeof(__int128_t));
        value = gbswap_128(value);
        value = value >> ((sizeof(value) - BINSZ) * 8);
        memcpy(dst_data, &value, sizeof(__int128_t));
        src_data += BINSZ;
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

static void SSENullableImpl(benchmark::State& state) {
    std::vector<uint8_t> srcs;
    std::vector<uint8_t> dsts;
    std::vector<uint8_t> nulls;

    srcs.resize(12 * 4096 + 15);
    dsts.resize(16 * 4096);
    nulls.resize(4096);

    for (auto _ : state) {
        sse_convert_nullable<12>(srcs.data(), nulls.data(), dsts.data(), 4096);
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

static void ScalarNullableImpl(benchmark::State& state) {
    std::vector<uint8_t> srcs;
    std::vector<uint8_t> dsts;
    std::vector<uint8_t> nulls;

    srcs.resize(12 * 4096 + 15);
    dsts.resize(16 * 4096);
    nulls.resize(4096);

    for (auto _ : state) {
        scalar_nullable_convert<12>(srcs.data(), nulls.data(), dsts.data(), 4096);
    }
}

BENCHMARK(ScalarImpl);
BENCHMARK(SSEImpl);
BENCHMARK(SSENullableImpl);
BENCHMARK(ScalarNullableImpl);
BENCHMARK_MAIN();

// -------------------------------------------------------------
// Benchmark                   Time             CPU   Iterations
// -------------------------------------------------------------
// ScalarImpl               4562 ns         4562 ns       153302
// SSEImpl                  2255 ns         2255 ns       310874
// SSENullableImpl          2170 ns         2170 ns       323321
// ScalarNullableImpl       6130 ns         6130 ns       110974