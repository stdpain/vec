#include <immintrin.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace starrocks {

// vectorwise ht
struct VectorWiseHashTable {
    std::vector<uint32_t> first;
    std::vector<uint32_t> next;
    // keys or payload
    std::vector<uint32_t> values;
    VectorWiseHashTable();

    void build_ht();

    size_t hash_compute(uint32_t element) { return element; }

    void batch_probe(const std::vector<uint32_t>& probe_values, std::vector<uint32_t>* build_rids);

private:
    size_t build_bucket_size() const {
        size_t total_elements = values.size();
        size_t buckets = total_elements ? ~size_t{} >> __builtin_clzll(total_elements) : 1;
        assert((buckets + 1 & buckets) == 0);
        return buckets;
    }
};

VectorWiseHashTable::VectorWiseHashTable() {
    first.emplace_back();
    next.emplace_back();
    values.emplace_back();
}

void VectorWiseHashTable::build_ht() {
    // allocate
    const size_t num_buckets = build_bucket_size();
    first.resize(num_buckets);
    next.resize(values.size());

    size_t build_size = values.size();

    for (size_t i = 1; i < build_size; ++i) {
        uint32_t bucket = hash_compute(values[i]) & num_buckets;
        next[i + 1] = first[bucket];
        first[bucket] = i + 1;
    }
}

void VectorWiseHashTable::batch_probe(const std::vector<uint32_t>& probe_values,
                                      std::vector<uint32_t>* build_rows) {
    const uint32_t* first_addr = first.data();
    const uint32_t* next_addr = next.data();
    __mmask16 masks = 0;
    __m512i inc = _mm512_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    __m512i probe_rids = _mm512_setzero_epi32();
    __m512i probe_load = _mm512_setzero_epi32();
    __m512i probe_buckets = _mm512_setzero_epi32();
    __m512i values = _mm512_setzero_epi32();
    const uint32_t* cursor = probe_values.data();
    const uint32_t* end_cursor = probe_values.data() + probe_values.size();
    while (cursor + 16 < end_cursor) {
        __mmask16 selected = ~masks;
        probe_load = _mm512_mask_expandloadu_epi32(probe_load, selected, cursor);
        probe_rids = _mm512_mask_expand_epi32(probe_rids, selected, inc);
        
        cursor += _mm_popcnt_u32(selected);
        // compute hash and bucket id

        // load first index for selected
        probe_buckets =
                _mm512_mask_i32gather_epi32(probe_buckets, selected, probe_buckets, first_addr, 4);

        // load next index
        probe_buckets =
                _mm512_mask_i32gather_epi32(probe_buckets, masks, probe_buckets, next_addr, 4);

        // load values
        values = _mm512_i32gather_epi32(probe_buckets, next_addr, 4);

        // compare value
        masks = _mm512_cmp_epi32_mask(probe_load, values, _MM_CMPINT_EQ);

        // store output by masks

        // return if full

        // load next mask
        probe_buckets = _mm512_i32gather_epi32(probe_buckets, next_addr, 4);
        masks = _mm512_cmp_epi32_mask(probe_buckets, _mm512_setzero_epi32(), _MM_CMPINT_EQ);
    }
    // TODO: process tail
}

} // namespace starrocks