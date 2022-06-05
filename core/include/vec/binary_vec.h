#pragma once
#include <string.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "vec/slice.h"

namespace vec {

class FlatBinaryVec {
public:
    FlatBinaryVec() : _offsets(1) {}
    Slice get_slice(int i) {
        return Slice(reinterpret_cast<char*>(_bytes.data()) + _offsets[i],
                     _offsets[i] - _offsets[i - 1]);
    }
    void build_strings(const std::vector<Slice>& slices);

private:
    std::vector<uint32_t> _offsets;
    std::vector<uint8_t> _bytes;
};

class InlineBinaryVec {
public:
    InlineBinaryVec() = default;
    void build_strings(const std::vector<Slice>& slices);
    SliceInline get_slice(int i) { return _datas[i]; }

private:
    std::vector<SliceInline> _datas;
    std::vector<uint8_t> _bytes;
};

} // namespace vec