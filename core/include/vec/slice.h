#include <emmintrin.h>
#include <smmintrin.h>
#include <sys/types.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace vec {

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
        return res;
    }
    return compare(size1, size2);
}

struct Slice {
    Slice() = default;
    Slice(char* data_, size_t size_) : data(data_), size(size_) {}
    Slice(const char* data_, size_t size_) : data(const_cast<char*>(data_)), size(size_) {}
    char* data;
    size_t size;

    int compare(const Slice& b) const;
};

inline bool memequal(const char* p1, size_t size1, const char* p2, size_t size2) {
    if (size1 != size2) {
        return false;
    }

    if (size1 == 0) {
        return true;
    }

    // const char * p1_end = p1 + size1;
    const char* p1_end_16 = p1 + size1 / 16 * 16;

    __m128i zero16 = _mm_setzero_si128();

    while (p1 < p1_end_16) {
        if (!_mm_testc_si128(zero16,
                             _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p1)),
                                           _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2)))))
            return false;

        p1 += 16;
        p2 += 16;
    }

    switch (size1 % 16) {
    case 15:
        if (p1[14] != p2[14]) return false;
        [[fallthrough]];
    case 14:
        if (p1[13] != p2[13]) return false;
        [[fallthrough]];
    case 13:
        if (p1[12] != p2[12]) return false;
        [[fallthrough]];
    case 12:
        if (reinterpret_cast<const uint32_t*>(p1)[2] == reinterpret_cast<const uint32_t*>(p2)[2])
            goto l8;
        else
            return false;
    case 11:
        if (p1[10] != p2[10]) return false;
        [[fallthrough]];
    case 10:
        if (p1[9] != p2[9]) return false;
        [[fallthrough]];
    case 9:
        if (p1[8] != p2[8]) return false;
    l8:
        [[fallthrough]];
    case 8:
        return reinterpret_cast<const uint64_t*>(p1)[0] == reinterpret_cast<const uint64_t*>(p2)[0];
    case 7:
        if (p1[6] != p2[6]) return false;
        [[fallthrough]];
    case 6:
        if (p1[5] != p2[5]) return false;
        [[fallthrough]];
    case 5:
        if (p1[4] != p2[4]) return false;
        [[fallthrough]];
    case 4:
        return reinterpret_cast<const uint32_t*>(p1)[0] == reinterpret_cast<const uint32_t*>(p2)[0];
    case 3:
        if (p1[2] != p2[2]) return false;
        [[fallthrough]];
    case 2:
        return reinterpret_cast<const uint16_t*>(p1)[0] == reinterpret_cast<const uint16_t*>(p2)[0];
    case 1:
        if (p1[0] != p2[0]) return false;
        [[fallthrough]];
    case 0:
        break;
    }

    return true;
}

/// Check whether two slices are identical.
inline bool operator==(const Slice& x, const Slice& y) {
    return x.size == y.size && memcmp(x.data, y.data, x.size);
    // return memequal(x.data, x.size, y.data, y.size);
}

/// Check whether two slices are not identical.
inline bool operator!=(const Slice& x, const Slice& y) {
    return !(x == y);
}

inline int Slice::compare(const Slice& b) const {
    return memcompare(data, size, b.data, b.size);
}

inline bool operator<(const Slice& lhs, const Slice& rhs) {
    return lhs.compare(rhs) < 0;
}

inline bool operator<=(const Slice& lhs, const Slice& rhs) {
    return lhs.compare(rhs) <= 0;
}

inline bool operator>(const Slice& lhs, const Slice& rhs) {
    return lhs.compare(rhs) > 0;
}

inline bool operator>=(const Slice& lhs, const Slice& rhs) {
    return lhs.compare(rhs) >= 0;
}

struct SliceInline {
    SliceInline() = default;
    SliceInline(char* data_, size_t size_)
            : SliceInline(reinterpret_cast<uint8_t*>(data_), size_) {}
    SliceInline(uint8_t* data_, int32_t size_) {
        memset(this, 0, sizeof(SliceInline));
        size = size_;
        if (size <= 12) {
            memcpy(prefix.data, data_, size_);
        } else {
            memcpy(prefix.data, data_, 4);
            data.pointer = reinterpret_cast<char*>(data_);
        }
    }

    int32_t size;
    union {
        int32_t i32;
        uint8_t data[4];
    } prefix;

    union {
        uint8_t datas[8];
        int64_t raw_data;
        char* pointer;
    } data;

    int compare(const Slice& b) const;
};

union UnionInt128 {
    SliceInline slice;
    __int128_t data;
};

inline bool operator==(const SliceInline& x, const SliceInline& y) {
    if (x.size != y.size) return false;
    if (x.size <= 12) {
        return (x.prefix.i32 == y.prefix.i32) & (x.data.raw_data == y.data.raw_data);
    }
    return x.prefix.i32 == y.prefix.i32 && memcmp(x.data.pointer, y.data.pointer, x.size);
}

} // namespace vec
