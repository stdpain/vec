#include <emmintrin.h>
#include <immintrin.h>

#include <cstddef>
#include <cstring>

#include "gtest/gtest.h"

inline int sse_memcmp2(const char* p1, const char* p2, size_t size) {
    __m128i left = _mm_lddqu_si128((__m128i*)(p1));
    __m128i right = _mm_lddqu_si128((__m128i*)(p2));
    __m128i nz = ~_mm_cmpeq_epi8(left, right);
    unsigned short mask = _mm_movemask_epi8(nz);
    int index = __builtin_ctz(mask);
    if (index >= size) return 0;
    return (int)(uint8_t)p1[index] - (int)(uint8_t)p2[index];
}

template <class T>
int vsigned(T v) {
    return (v >> (sizeof(T) * 8 - 1)) & 1;
}

TEST(sse_memcmp, Test) {
    ASSERT_EQ(vsigned((int)-1), 1);
    ASSERT_EQ(vsigned((int)1), 0);

    {
        const char c1[32] = "ABC";
        const char c2[32] = "CBA";

        int res = memcmp(c1, c2, 3);
        int res2 = sse_memcmp2(c1, c2, 3);
        ASSERT_EQ(vsigned(res), vsigned(res2));
    }
    {
        const char c1[32] = "AAB";
        const char c2[32] = "ABA";

        int res = memcmp(c1, c2, 3);
        int res2 = sse_memcmp2(c1, c2, 3);
        ASSERT_EQ(vsigned(res), vsigned(res2));
    }
    {
        const char c1[32] = "ABC";
        const char c2[32] = "ABA";

        int res = memcmp(c1, c2, 3);
        int res2 = sse_memcmp2(c1, c2, 3);
        ASSERT_EQ(vsigned(res), vsigned(res2));
    }
    {
        const char c1[32] = "ABC";
        const char c2[32] = "ABC";

        int res = memcmp(c1, c2, 3);
        int res2 = sse_memcmp2(c1, c2, 3);
        ASSERT_EQ(res, res2);
    }

    {
        const char c1[32] = "ABC";
        const char c2[32] = "你好";

        int res = memcmp(c1, c2, 3);
        int res2 = sse_memcmp2(c1, c2, 3);
        ASSERT_EQ(vsigned(res), vsigned(res2));
    }

    {
        const char c1[32] = "";
        const char c2[32] = "你好";

        int res = memcmp(c1, c2, 3);
        int res2 = sse_memcmp2(c1, c2, 3);
        ASSERT_EQ(vsigned(res), vsigned(res2));
    }

    {
        const char c1[32] = "0123456789abcdef";
        const char c2[32] = "0123456789abcdff";

        int res = memcmp(c1, c2, 3);
        int res2 = sse_memcmp2(c1, c2, 3);
        ASSERT_EQ(vsigned(res), vsigned(res2));
    }

    {
        const char c1[32] = "0123456789abcdef";
        const char c2[32] = "0123456789abcdef";

        int res = memcmp(c1, c2, 3);
        int res2 = sse_memcmp2(c1, c2, 3);
        ASSERT_EQ(res, res2);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}