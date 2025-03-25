#include <byteswap.h>
#include <immintrin.h>

#include <cstddef>
#include <cstring>

#include "gtest/gtest.h"

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
void convert(const uint8_t* __restrict src_null_data, const uint8_t* __restrict dst_data,
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

inline unsigned __int128 gbswap_128(unsigned __int128 host_int) {
    return static_cast<unsigned __int128>(bswap_64(static_cast<uint64_t>(host_int >> 64))) |
           (static_cast<unsigned __int128>(bswap_64(static_cast<uint64_t>(host_int))) << 64);
}

TEST(TestMASK, MASK_GEN) {
    {
        __m128i mask1 = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
        __m128i mask2 = generate_shuf_msk<16>();
        ASSERT_EQ(0, memcmp(&mask1, &mask2, sizeof(mask1)));
    }
    {
        __m128i mask1 = _mm_set_epi8(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0);
        __m128i mask2 = generate_shuf_msk<1>();
        ASSERT_EQ(0, memcmp(&mask1, &mask2, sizeof(mask1)));
    }

    {
        __m128i mask1 = _mm_set_epi8(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                     0xFF, 0xFF, 0xFF, 0xFF, 0, 1);
        __m128i mask2 = generate_shuf_msk<2>();
        ASSERT_EQ(0, memcmp(&mask1, &mask2, sizeof(mask1)));
    }

    {
        __m128i mask1 = _mm_set_epi8(0xFF, 0xFF, 0xFF, 0xFF, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
        __m128i mask2 = generate_shuf_msk<12>();
        ASSERT_EQ(0, memcmp(&mask1, &mask2, sizeof(mask1)));
    }
}

TEST(TestDecimal, SSE_CVT) {
    __int128_t a = 0x123456789abcdef0;
    a = (a << 8) | (0xf0efcba987654321);
#define ADD_CASE(bin_sz)                                \
    {                                                   \
        constexpr int BINSZ = bin_sz;                   \
        __int128_t value = gbswap_128(a);               \
        value = value >> ((sizeof(value) - BINSZ) * 8); \
                                                        \
        __int128_t b;                                   \
        __m128i mask = generate_shuf_msk<BINSZ>();      \
        __m128i xmm = _mm_loadu_si128((__m128i*)&a);    \
        xmm = _mm_shuffle_epi8(xmm, mask);              \
        _mm_storeu_si128((__m128i*)&b, xmm);            \
        ASSERT_EQ(value, b);                            \
    }

    ADD_CASE(1)
    ADD_CASE(2)
    ADD_CASE(3)
    ADD_CASE(4)
    ADD_CASE(5)
    ADD_CASE(6)
    ADD_CASE(7)
    ADD_CASE(8)
    ADD_CASE(9)
    ADD_CASE(10)
    ADD_CASE(11)
    ADD_CASE(12)
    ADD_CASE(13)
    ADD_CASE(14)
    ADD_CASE(15)
    ADD_CASE(16)
#undef ADD_CASE
}

TEST(TestSWAP, SSESWAP) {
    __int128_t a = 0x123456789abcdef0;
    a = (a << 8) | (0xf0efcba987654321);
    __int128_t swap = gbswap_128(a);
    __m128i mask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    __m128i shf = _mm_loadu_si128((__m128i*)&a);
    shf = _mm_shuffle_epi8(shf, mask);
    __int128_t b;
    _mm_storeu_si128((__m128i*)&b, shf);
    ASSERT_EQ(swap, b);
}

TEST(TestNull, BatchCheckNull) {
    std::vector<uint8_t> nulls;
    nulls.resize(100);
    auto* raw_null = nulls.data();

    nulls[0] = 0;
    nulls[1] = 0;

    nulls[2] = 0;
    nulls[3] = 1;

    nulls[4] = 1;
    nulls[5] = 0;

    nulls[6] = 1;
    nulls[7] = 1;

    uint16_t val;
    memcpy(&val, raw_null, sizeof(val));
    ASSERT_EQ(val, 0);
    memcpy(&val, raw_null + 2, sizeof(val));
    ASSERT_EQ(val, 0x0100);
    memcpy(&val, raw_null + 4, sizeof(val));
    ASSERT_EQ(val, 0x0001);
    memcpy(&val, raw_null + 6, sizeof(val));
    ASSERT_EQ(val, 0x0101);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}