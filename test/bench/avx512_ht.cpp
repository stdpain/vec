#include <immintrin.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>

// vectorwise ht
struct VectorWiseHashTable {
    std::vector<uint32_t> first;
    std::vector<uint32_t> next;
    // cmp keys
    std::vector<uint32_t> values;

    VectorWiseHashTable();

    void build_ht();

    size_t hash_compute(uint32_t element) { return element; }

    size_t batch_probe(const std::vector<uint32_t>& probe_values,
                       std::vector<uint32_t>* build_rids);

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
        next[i] = first[bucket];
        first[bucket] = i;
    }
}

// AVX512 Join Hash table vertical probe implements
size_t VectorWiseHashTable::batch_probe(const std::vector<uint32_t>& probe_values,
                                        std::vector<uint32_t>* build_rows) {
    const uint32_t* first_addr = first.data();
    const uint32_t* next_addr = next.data();
    const uint32_t* values_addr = values.data();
    __mmask16 masks = 0;
    __m512i inc = _mm512_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    __m512i probe_rids = _mm512_setzero_epi32();
    __m512i probe_load = _mm512_setzero_epi32();
    __m512i probe_buckets = _mm512_setzero_epi32();
    __m512i values = _mm512_setzero_epi32();
    const uint32_t* cursor = probe_values.data();
    const uint32_t* end_cursor = probe_values.data() + probe_values.size();
    const size_t num_buckets = build_bucket_size();
    __m512i mul = _mm512_set1_epi32(num_buckets);
    uint32_t* dst = build_rows->data();
    size_t buffer_size = build_rows->size() / 2;
    size_t cnt = 0;
    while (cursor + 16 <= end_cursor && cnt + 16 <= buffer_size) {
        __mmask16 selected = ~masks;
        probe_load = _mm512_mask_expandloadu_epi32(probe_load, selected, cursor);
        probe_rids = _mm512_mask_expand_epi32(probe_rids, selected, inc);

        cursor += _mm_popcnt_u32(selected);
        // compute hash and bucket id
        __m512i hash_values = probe_load;
        probe_buckets = _mm512_and_epi32(hash_values, mul);

        // load first index for selected
        probe_buckets =
                _mm512_mask_i32gather_epi32(probe_buckets, selected, probe_buckets, first_addr, 4);

        // load next index
        probe_buckets =
                _mm512_mask_i32gather_epi32(probe_buckets, masks, probe_buckets, next_addr, 4);

        // skip probe_bucket[i] == 0
        __mmask16 k1 = _mm512_cmp_epi32_mask(probe_buckets, _mm512_setzero_epi32(), _MM_CMPINT_NE);

        // load values
        values = _mm512_i32gather_epi32(probe_buckets, values_addr, 4);

        // compare value
        __mmask16 k2 = _mm512_mask_cmp_epi32_mask(k1, probe_load, values, _MM_CMPINT_EQ);

        // store output by masks
        // store probe_id-build_id to build_rows
        __m512i hi = _mm512_unpackhi_epi32(probe_load, probe_buckets);
        __m512i lo = _mm512_unpacklo_epi32(probe_load, probe_buckets);
        lo = _mm512_maskz_compress_epi64(k2 >> 8, lo);
        hi = _mm512_maskz_compress_epi64(k2, hi);
        _mm512_storeu_epi64(dst + cnt * 2, lo);
        _mm512_storeu_epi64(dst + cnt * 2 + 16, hi);
        cnt += _mm_popcnt_u32(k2);

        // load next mask
        masks = !k1;
    }

    if (cursor < end_cursor && cnt + 16 <= buffer_size) {
        __mmask16 lefted = ~masks;
        int32_t probe_load_arr[16];
        int32_t probe_rid_arr[16];
        _mm512_storeu_epi32(probe_load_arr, probe_load);
        _mm512_storeu_epi32(probe_rid_arr, probe_rids);
        for (; lefted != 0;) {
            // process
            int32_t next_id = __builtin_ctzll(lefted);
            int32_t probe_val = probe_load_arr[next_id];
            int32_t probe_rid = probe_rid_arr[next_id];
            while (next_id != 0) {
                if (values_addr[next_id] == probe_val) {
                    dst[cnt * 2] = probe_rid;
                    dst[cnt * 2 + 1] = next_id;
                    cnt++;
                }
                next_id = next_addr[next_id];
            }

            lefted = lefted & (lefted - 1);
        }
    }
    // TODO: save unfinished status

    return cnt;
}

int main(int argc, char* argv[]) {
    // basic test
    VectorWiseHashTable ht;
    for (size_t i = 0; i < 4096; ++i) {
        ht.values.emplace_back(i);
    }
    ht.build_ht();

    std::vector<uint32_t> probe_values;
    for (size_t i = 0; i < 4096; ++i) {
        probe_values.emplace_back(i);
    }
    std::vector<uint32_t> dst;
    dst.resize(probe_values.size() * 2);

    size_t v = ht.batch_probe(probe_values, &dst);
    std::cout << v << std::endl;
    std::set<int> vals;
    for (size_t i = 0; i < dst.size(); i += 2) {
        if (probe_values[dst[i]] != ht.values[dst[i + 1]]) {
            std::abort();
        }
        vals.insert(probe_values[dst[i]]);
        std::cout << probe_values[dst[i]] << std::endl;
    }
    if (vals.size() != 4096) {
        std::abort();
        size_t expected = 0;
        for (auto v : vals) {
            if (v != expected) {
                std::abort();
            }
            expected++;
        }
    }

    return 0;
}
