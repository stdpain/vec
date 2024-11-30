#pragma once

#include <iterator>

namespace vec {
template <class T>
class VecIterator {
public:
    // https://stackoverflow.com/questions/43268146/why-is-stditerator-deprecated
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = T;
    using pointer = T*;
    using reference = T&;

    VecIterator(T* pos) : _ptr(pos) {}
    VecIterator& operator=(const VecIterator& iter) {
        _ptr = iter._ptr;
        return *this;
    }
    bool operator!=(const VecIterator& iter) const { return _ptr != iter._ptr; }
    bool operator==(const VecIterator& iter) const { return _ptr == iter._ptr; }
    bool operator<(const VecIterator& iter) const { return _ptr < iter._ptr; }
    VecIterator operator+(int pos) { return VecIterator(_ptr + pos); }
    VecIterator operator-(int pos) { return VecIterator(_ptr - pos); }
    VecIterator& operator++() {
        _ptr++;
        return *this;
    }
    VecIterator operator--() { return VecIterator(_ptr--); }
    T& operator*() { return *_ptr; }
    const T& operator*() const { return *_ptr; }
    int operator-(const VecIterator& iter) const { return _ptr - iter._ptr; }

private:
    T* _ptr;
};
} // namespace vec
