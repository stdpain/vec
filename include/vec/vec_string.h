#pragma once

#include <cstdlib>
#include <cstring>
#include <string>

#include "vec/vec.h"
#include "vec/vec_iterator.h"

namespace vec {

struct FixedString {};

template <>
class Vec<FixedString> {
public:
    Vec(int size, int len) : _size(size), _capacity(len), _len(len) {
        _data = new (std::nothrow) char[_size * _capacity];
        memset(_data, 0, _size * _capacity);
    }
    ~Vec() { delete[] _data; }

    Vec(const Vec& other) {
        _capacity = other.len();
        _len = other.len();
        _data = new (std::nothrow) char[_size * _capacity];
        memcpy(_data, other._data, _size * _len);
    }

    Vec& operator=(const Vec& other) {
        _capacity = other.len();
        _len = other.len();
        _data = new (std::nothrow) char[_size * _capacity];
        memcpy(_data, other._data, _size * _len);
        return *this;
    }

    Vec& operator=(const std::string& other) {
        VEC_ASSERT_TRUE(other.length() <= _size);
        char* start = _data;
        char* end = _data + _size * _len;
        int i = 0;
        while (start < end) {
            memcpy(start, other.c_str(), other.size());
            start += _size;
        }
        return *this;
    }

    Vec& operator=(Vec&& other) {
        std::swap(_size, other._size);
        std::swap(_len, other._len);
        std::swap(_capacity, other._capacity);
        std::swap(_data, other._data);
        return *this;
    }

    Vec(Vec&& other) {
        _size = 0;
        _len = 0;
        _capacity = 0;
        _data = nullptr;
        std::swap(_size, other._size);
        std::swap(_len, other._len);
        std::swap(_capacity, other._capacity);
        std::swap(_data, other._data);
    }

    int fixed_size() const { return _size; }

    int len() const { return _len; }

    int capacity() const { return _capacity; }

    Vec<int> strlen() const {
        Vec<int> len_vec(_len);
        char* start = _data;
        char* end = _data + _size * _len;
        int i = 0;
        while (start < end) {
            len_vec.data()[i] = strnlen(start, _size);
            start += _size;
        }
        return len_vec;
    };

    char* operator[](const int index) {
        VEC_ASSERT_TRUE(index < _len);
        return &_data[index * _size];
    }

    char* at(const int index) {
        return &_data[index * _size];
    }

    // support std::string_view, std::string
    template <typename StringType>
    void set(const StringType& value, int index) {
        VEC_ASSERT_TRUE(index < _len);
        memcpy(this->at(index), value.c_str(), value.length());
    }
    // TODO:push_back

private:
    // fixed string size
    int _size;
    int _capacity;
    int _len;
    char* _data;
};

} // namespace vec
