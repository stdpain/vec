#include <limits>
#pragma GCC option("arch=native", "tune=native", "no-zero-upper") //Enable AVX2
#pragma GCC target("avx2")                                        //Enable AVX2
#include <x86intrin.h>                                            //AVX/SSE Extensions

#include <cstdio>
#include <iostream>

#include "gtest/gtest.h"

TEST(SIMDTest, BasicTest) {
    // create a vector all element equals to 3.14
    __m256 fltx4 = _mm256_set1_ps(3.14f);
    fltx4[0] = 1.32f;   // valid
    float f = fltx4[1]; // valid

    ASSERT_FLOAT_EQ(f, 3.14f);
    ASSERT_FLOAT_EQ(fltx4[0], 1.32f);

    // create a int type vector
    __m256i int32x16 = _mm256_set1_epi32(0x3355ff);
    int i = _mm256_extract_epi32(int32x16, 0);
    int16_t i16 = _mm256_extract_epi32(int32x16, 0);

    ASSERT_EQ(0x3355ff, i);
    ASSERT_EQ(0x55ff, i16);

    short short_maxn = std::numeric_limits<short>::max();
    __m256i int16x16_1 = _mm256_set1_epi16(short_maxn - 1000);
    __m256i int16x16_2 = _mm256_set1_epi16(short_maxn - 1000);
    __m256i res = _mm256_adds_epi16(int16x16_1, int16x16_2);
    int16_t ri16 = _mm256_extract_epi32(res, 0);
    // adds operate overflow will set to maxn
    ASSERT_EQ(ri16, short_maxn);

    short overflow_res = std::numeric_limits<short>::max() + std::numeric_limits<short>::max();
    ASSERT_NE(ri16, overflow_res);

    auto res0 = _mm256_add_epi8(_mm256_set1_epi8(100), _mm256_set1_epi8(100));
    int8_t v1 = _mm256_extract_epi8(res0, 0);
    int8_t v2 = 100 + 100; // overflow

    ASSERT_EQ(v1, v2); // add operate will overflow

    {
        float fary[4] = {1, 2, 3, 4};
        // load data from float array
        __m128 f = _mm_loadu_ps(fary);
        // store vector to flot array
        _mm_storeu_ps(fary, f);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
