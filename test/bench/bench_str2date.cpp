#define BENCHMARK_HAS_CXX11
#include <benchmark/benchmark.h>
#include <smmintrin.h>

#include <cstring>
#include <iostream>

namespace simd {
// Parse "YYYY-MM-DD" using SIMD without storing intermediate results.
//
// Key idea:
//   We cannot directly compute
//     year = d0*1000 + d1*100 + d2*10 + d3
//   because _mm_maddubs_epi16 only supports int8 weights and operates
//   on pairs of bytes.
//
//   Instead, we rewrite the formula as:
//     year  = (d0*10 + d1) * 100 + (d2*10 + d3)
//     month =  d5*10 + d6
//     day   =  d8*10 + d9
//
//   This decomposition:
//     - matches the semantics of pmaddubsw (pairwise multiply-add)
//     - keeps all intermediate values within int16 range
//     - allows larger weights (100) to be applied in the second stage
//       using pmaddwd (int16 * int16 -> int32)
//
// Layout after ASCII -> digit conversion:
//
//   index:   0  1  2  3  4  5  6  7  8  9
//   input:   Y  Y  Y  Y  -  M  M  -  D  D
//   digit:  d0 d1 d2 d3  x d5 d6  x d8 d9
//

inline __m128i load16_safe(const char* p) {
    constexpr uintptr_t PAGE_MASK = 4096 - 1;
#ifndef ADDRESS_SANITIZER
    bool read_cross_page = (reinterpret_cast<uintptr_t>(p) & PAGE_MASK) > PAGE_MASK - 16;
#else
    bool read_cross_page = false;
#endif
    if (!read_cross_page) {
        return _mm_loadu_si128((const __m128i*)p);
    } else {
        alignas(16) char tmp[16] = {};
        memcpy(tmp, p, 10);
        return _mm_load_si128((const __m128i*)tmp);
    }
}

inline bool from_string_to_date_internal(const char* ptr, int* year, int* month, int* day) {
    // // Load 16 bytes; we only care about the first 10 ("YYYY-MM-DD")

    __m128i v = load16_safe(ptr);

    // ------------------------------------------------------------
    // 1. Validate digit positions (ASCII '0'..'9')
    // ------------------------------------------------------------
    const __m128i zero = _mm_set1_epi8('0');
    const __m128i nine = _mm_set1_epi8('9');

    __m128i ge0 = _mm_cmpgt_epi8(v, _mm_sub_epi8(zero, _mm_set1_epi8(1)));
    __m128i le9 = _mm_cmpgt_epi8(_mm_add_epi8(nine, _mm_set1_epi8(1)), v);
    __m128i is_digit = _mm_and_si128(ge0, le9);

    // Expected digit mask for "YYYY-MM-DD"
    // digit positions: 0,1,2,3,5,6,8,9
    constexpr int DIGIT_MASK = 0b0000001101101111;
    if ((_mm_movemask_epi8(is_digit) & 0x3FF) != DIGIT_MASK) return false;

    // ------------------------------------------------------------
    // 2. Convert ASCII digits to numeric values (0..9)
    // ------------------------------------------------------------
    __m128i digits = _mm_sub_epi8(v, zero);

    // ------------------------------------------------------------
    // 3. First stage: pairwise digit aggregation using pmaddubsw
    //
    // Each pair computes:
    //   (d0*10 + d1), (d2*10 + d3), (d5*10 + d6), (d8*10 + d9)
    //
    // All results fit in int16 (max 99).
    // ------------------------------------------------------------
    const __m128i w1 = _mm_setr_epi8(10, 1, // d0*10 + d1
                                     10, 1, // d2*10 + d3
                                     0, 0,  // '-'
                                     10, 1, // d5*10 + d6
                                     0, 0,  // '-'
                                     10, 1, // d8*10 + d9
                                     0, 0, 0, 0);

    __m128i t16 = _mm_maddubs_epi16(digits, w1);

    // ------------------------------------------------------------
    // 4. Second stage: combine pairs into final values using pmaddwd
    //
    //   year  = (d0*10 + d1) * 100 + (d2*10 + d3)
    //   month = (d5*10 + d6)
    //   day   = (d8*10 + d9)
    //
    // This stage uses int16 * int16 -> int32, allowing larger weights.
    // ------------------------------------------------------------
    const __m128i w2 = _mm_setr_epi16(100, 1, // year
                                      0, 1,   // month
                                      0, 1,   // day
                                      0, 0);

    __m128i t32 = _mm_madd_epi16(t16, w2);

    // ------------------------------------------------------------
    // 5. Extract results (no intermediate stores)
    // ------------------------------------------------------------
    *year = _mm_extract_epi32(t32, 0);
    *month = _mm_extract_epi32(t32, 1);
    *day = _mm_extract_epi32(t32, 2);

    return true;
}

} // namespace simd
namespace scalar {
bool from_string_to_date_internal(const char* ptr, int* pyear, int* pmonth, int* pday) {
    // Fallback non-SIMD version
    const bool is_valid = isdigit(ptr[0]) && isdigit(ptr[1]) && isdigit(ptr[2]) &&
                          isdigit(ptr[3]) && !isdigit(ptr[4]) && isdigit(ptr[5]) &&
                          isdigit(ptr[6]) && !isdigit(ptr[7]) && isdigit(ptr[8]) && isdigit(ptr[9]);
    if (!is_valid) {
        return false;
    }

    const int year =
            ptr[0] * 1000 + ptr[1] * 100 + ptr[2] * 10 + ptr[3] - static_cast<int>('0') * 1111;
    const int month = ptr[5] * 10 + ptr[6] - static_cast<int>('0') * 11;
    const int day = ptr[8] * 10 + ptr[9] - static_cast<int>('0') * 11;

    *pyear = year;
    *pmonth = month;
    *pday = day;

    return true;
}
} // namespace scalar

// Benchmark for string to date parsing (YYYY-MM-DD format)
template <bool call(const char* ptr, int* pyear, int* pmonth, int* pday)>
static void BM_StringToDate_StandardFormat(benchmark::State& state) {
    std::vector<std::string> date_strings = {
            "2024-01-15", "2023-12-31", "2024-02-29", "1990-06-15", "2025-08-20",
            "2024-03-10", "2023-11-25", "2024-07-04", "1988-04-08", "2026-05-17",
    };

    int year, month, day;
    size_t idx = 0;
    size_t items_processed = 0;

    for (auto _ : state) {
        const std::string& date_str = date_strings[idx % date_strings.size()];
        bool success = call(date_str.c_str(), &year, &month, &day);
        benchmark::DoNotOptimize(success);
        benchmark::DoNotOptimize(year);
        benchmark::DoNotOptimize(month);
        benchmark::DoNotOptimize(day);
        idx++;
        items_processed++;
    }

    state.SetItemsProcessed(items_processed);
}

// -------------------------------------------------------------------------------------------------------------------------------
// Benchmark                                                                     Time             CPU   Iterations UserCounters...
// -------------------------------------------------------------------------------------------------------------------------------
// BM_StringToDate_StandardFormat<simd::from_string_to_date_internal>         10.3 ns         10.2 ns     68212555 items_per_second=97.5621M/s
// BM_StringToDate_StandardFormat<scalar::from_string_to_date_internal>       14.0 ns         14.0 ns     49763048 items_per_second=71.3602M/s

BENCHMARK_TEMPLATE(BM_StringToDate_StandardFormat, simd::from_string_to_date_internal);
BENCHMARK_TEMPLATE(BM_StringToDate_StandardFormat, scalar::from_string_to_date_internal);

BENCHMARK_MAIN();
