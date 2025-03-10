#include <benchmark/benchmark.h>
#include <emmintrin.h>
#include <immintrin.h>

#include <algorithm>
#include <cstring>
#include <numeric>
#include <random>
#include <string_view>
#include <vector>

constexpr int string_length = 8;
constexpr int num_rows = 8;

using Strings = std::vector<std::string_view>;
using Dsts = std::vector<int>;

template <typename T>
inline int compare(T lhs, T rhs) {
    if (lhs < rhs) {
        return -1;
    } else if (lhs > rhs) {
        return 1;
    } else {
        return 0;
    }
}

inline int memcompare(const char* p1, size_t size1, const char* p2, size_t size2) {
    size_t min_size = std::min(size1, size2);
    auto res = memcmp(p1, p2, min_size);
    if (res != 0) {
        return res > 0 ? 1 : -1;
    }
    return compare(size1, size2);
}

template <typename T>
inline int compare2(T lhs, T rhs) {
    return (lhs > rhs) - (lhs < rhs);
}

inline int sse_memcmp(const char* p1, size_t size1, const char* p2, size_t size2) {
    __m128i zero = _mm_setzero_si128();
    // Create a mask with the least significant 'size' bits set to 1
    __mmask16 mask1 = (1 << size1) - 1;
    __m128i left = _mm_mask_loadu_epi8(zero, mask1, static_cast<const char*>(p1));
    __mmask16 mask2 = (1 << size2) - 1;
    __m128i right = _mm_mask_loadu_epi8(zero, mask2, static_cast<const char*>(p2));
    // const int simd_compare_string_mode = _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_NEGATIVE_POLARITY | _SIDD_BIT_MASK;
    // return _mm_cmpestri(left, size1, right , size2, simd_compare_string_mode);
    __m128i sub = _mm_sub_epi8(left, right);

    unsigned short mask = _mm_movemask_epi8(sub);
    if (mask == 0) {
        return 0;
    }
    int index = __builtin_ctz(mask);
    alignas(16) int8_t bytes[16]; // or uint8_t
    _mm_store_si128((__m128i*)bytes, sub);
    return bytes[index];
}

// only work in ascii cmp
inline int sse_memcmp2(const char* p1, size_t size1, const char* p2, size_t size2) {
    __m128i left = _mm_lddqu_si128((__m128i*)(p1));
    __m128i right = _mm_lddqu_si128((__m128i*)(p2));
    __m128i sub = _mm_sub_epi8(left, right);
    __m128i nz = ~_mm_cmpeq_epi8(sub, _mm_setzero_si128());
    unsigned short mask = _mm_movemask_epi8(nz);
    int index = __builtin_ctz(mask);
    alignas(16) int8_t bytes[16];
    _mm_store_si128((__m128i*)bytes, sub);
    if (index > size1) return 0;
    return bytes[index];
}

inline int sse_memcmp3(const char* p1, const char* p2, size_t size) {
    __m128i left = _mm_lddqu_si128((__m128i*)(p1));
    __m128i right = _mm_lddqu_si128((__m128i*)(p2));
    __m128i nz = ~_mm_cmpeq_epi8(left, right);
    unsigned short mask = _mm_movemask_epi8(nz);
    int index = __builtin_ctz(mask);
    if (index > size) return 0;
    return (int)(uint8_t)p1[index] - (int)(uint8_t)p2[index];
}

inline int memcompare2(const char* p1, size_t size1, const char* p2, size_t size2) {
    size_t min_size = std::min(size1, size2);
    if (min_size <= 16) {
        // auto res =  sse_memcmp(p1, size1, p2, size2);
        // auto res =  sse_memcmp2(p1, size1, p2, size2);
        auto res = sse_memcmp3(p1, p2, size2);
        if (res == 0) return compare(size1, size2);
        return res > 0 ? 1 : -1;
    }
    auto res = memcmp(p1, p2, min_size);
    if (res != 0) {
        return res > 0 ? 1 : -1;
    }
    return compare(size1, size2);
}

class Compare {
public:
    virtual void cmp(const Strings& v1, Strings& v2, Dsts* res) = 0;
};

class LibcMemCmp : public Compare {
public:
    virtual void cmp(const Strings& v1, Strings& v2, Dsts* res) {
        auto dst = *res;
        size_t num_rows = v1.size();
        for (size_t i = 0; i < num_rows; ++i) {
            dst[i] = memcompare(v1[i].data(), v1.size(), v2[i].data(), v2.size());
        }
    }
};

class SSEMemCmp : public Compare {
public:
    virtual void cmp(const Strings& v1, Strings& v2, Dsts* res) {
        auto dst = *res;
        size_t num_rows = v1.size();
        for (size_t i = 0; i < num_rows; ++i) {
            dst[i] = sse_memcmp(v1[i].data(), v1.size(), v2[i].data(), v2.size());
        }
    }
};

struct Context {
    Strings c1;
    Strings c2;
    Dsts dsts;
};

const char buffer1[32] = "TRUCK";
const char buffer2[32] = "TRUCC";

Context init_ctx(size_t num_rows) {
    Context context;

    context.c1.resize(num_rows);
    context.c2.resize(num_rows);
    context.dsts.resize(num_rows);

    for (size_t i = 0; i < num_rows; ++i) {
        context.c1[i] = buffer1;
        context.c2[i] = buffer2;
    }
    return context;
}

static void GlibcCmpImpl(benchmark::State& state) {
    Compare* c = new LibcMemCmp();
    Context v = init_ctx(num_rows);
    for (auto _ : state) {
        c->cmp(v.c1, v.c2, &v.dsts);
        benchmark::DoNotOptimize(v.dsts);
    }
}

static void SSEMemCmpImpl(benchmark::State& state) {
    Compare* c = new SSEMemCmp();
    Context v = init_ctx(num_rows);
    for (auto _ : state) {
        c->cmp(v.c1, v.c2, &v.dsts);
        benchmark::DoNotOptimize(v.dsts);
    }
}

BENCHMARK(GlibcCmpImpl);
BENCHMARK(SSEMemCmpImpl);
BENCHMARK_MAIN();

// --------------------------------------------------------
// Benchmark              Time             CPU   Iterations
// --------------------------------------------------------
// GlibcCmpImpl        40.2 ns         40.2 ns     17368765
// SSEMemCmpImpl       25.4 ns         25.4 ns     27569981