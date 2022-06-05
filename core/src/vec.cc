#include "vec/vec.h"

#include "vec/binary_vec.h"

namespace vec {

void FlatBinaryVec::build_strings(const std::vector<Slice>& slices) {
    size_t buffer_size = 0;
    for (size_t i = 0; i < slices.size(); ++i) {
        buffer_size += slices[i].size >= 12 ? slices[i].size : 0;
    }
    _offsets.reserve(slices.size());
    _bytes.reserve(buffer_size);

    for (const auto& s : slices) {
        const uint8_t* const p = reinterpret_cast<uint8_t*>(s.data);
        _bytes.insert(_bytes.end(), p, p + s.size);
        _offsets.emplace_back(_bytes.size());
    }
}

void InlineBinaryVec::build_strings(const std::vector<Slice>& slices) {
    size_t buffer_size = 0;
    for (size_t i = 0; i < slices.size(); ++i) {
        buffer_size += slices[i].size >= 12 ? slices[i].size : 0;
    }
    _datas.reserve(slices.size());
    _bytes.reserve(buffer_size);

    for (size_t i = 0; i < slices.size(); ++i) {
        const auto& s = slices[i];
        uint8_t* p = reinterpret_cast<uint8_t*>(s.data);
        if (s.size <= 12) {
            _datas.emplace_back(p, s.size);
        } else {
            auto* data = _bytes.data();
            _bytes.insert(_bytes.end(), p, p + s.size);
            _datas.emplace_back(data, s.size);
        }
    }
}

} // namespace vec