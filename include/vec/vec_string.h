#pragma once

#include <cstdlib>
#include <cstring>
#include <string>

#include "vec/vec.h"
#include "vec/vec_iterator.h"

namespace vec {

struct FixedString {
    char data[0];
    template <typename StringType>
    FixedString& operator=(const StringType& str) {
        memcpy(data, str.c_str(), str.length());
        return *this;
    }
};

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
            len_vec.data()[i++] = strnlen(start, _size);
            start += _size;
        }
        return len_vec;
    };

    FixedString& operator[](const int index) const {
        VEC_ASSERT_TRUE(index < _len);
        return this->at(index);
    }

    FixedString& operator[](const int index) {
        VEC_ASSERT_TRUE(index < _len);
        return this->at(index);
    }

    FixedString& at(const int index) const {
        return *reinterpret_cast<FixedString*>(&_data[index * _size]);
    }

    FixedString& at(const int index) {
        return *reinterpret_cast<FixedString*>(&_data[index * _size]);
    }

    // support std::string_view, std::string
    template <typename StringType>
    void set(const StringType& value, int index) {
        VEC_ASSERT_TRUE(index < _len);
        VEC_ASSERT_TRUE(value.length() <= _size);
        memset(this->at(index).data, 0, _size);
        memcpy(this->at(index).data, value.c_str(), value.length());
    }

    void reserve(int len) {
        VEC_ASSERT_TRUE(len >= 0);
        if (_capacity < len) {
            char* old_data = _data;
            _data = new (std::nothrow) char[_size * len];
            memcpy(_data, old_data, _size * _len);
            delete[] old_data;
            _capacity = len;
        }
    }

    void apply(std::function<void(FixedString&, int size)>& func) {
        for (int i = 0; i < _len; ++i) {
            func(this->at(i), _size);
        }
    }

    // example:
    // struct OP {
    // static void to_upper(FixedString& ,int length);
    // }
    template <typename OP>
    void apply() {
        for (int i = 0; i < _len; ++i) {
            OP::apply(this->at(i), _size);
        }
    }

    template <class U>
    Vec<U> transform(std::function<void(const FixedString&, U&, int)>& func) const {
        Vec<U> vec(_len);
        for (int i = 0; i < _len; ++i) {
            func(this->at(i), vec.at(i), _size);
        }
        return vec;
    }

    template <typename OP>
    Vec vec_transform() const {
        Vec vec(_size, _len);
        for (int i = 0; i < _len; ++i) {
            OP::apply(this->at(i), vec.at(i), _size);
        }
        return vec;
    }

    template <class StringType>
    void push_back(const StringType& t) {
        if (_len == _capacity) {
            reserve(_capacity * 2 + 1);
        }
        memcpy(this->at(_len++).data, t.c_str(), t.length());
    }

    void clear() { _len = 0; }

    // TODO: compare, function, iterator

private:
    // fixed string size
    int _size;
    int _capacity;
    int _len;
    char* _data;
};

} // namespace vec
