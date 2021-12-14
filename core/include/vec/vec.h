#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <new>

#include "vec/vec_iterator.h"

#ifndef VEC_ASSERT_TRUE
#define VEC_ASSERT_TRUE assert
#endif

namespace vec {

template <typename T>
class Vec {
public:
    Vec() : _capacity(0), _len(0), _data(nullptr){};

    Vec(int len) : _capacity(len), _len(len) {
        _data = new (std::nothrow) T[_len];
        memset(_data, 0, sizeof(T) * _len);
    }

    Vec(int len, T* data) : _capacity(len), _len(len), _data(data) {}

    Vec(int capacity, int len, T* data) : _capacity(capacity), _len(len), _data(data) {}

    ~Vec() { delete[] _data; };

    Vec(const Vec<T>& other) {
        _capacity = other.len();
        _len = other.len();
        _data = new (std::nothrow) T[_capacity];
        memcpy(_data, other._data, sizeof(T) * _len);
    }

    Vec<T>& operator=(const Vec<T>& other) {
        _capacity = other.len();
        _len = other.len();
        _data = new (std::nothrow) T[_capacity];
        memcpy(_data, other._data, sizeof(T) * _len);
        return *this;
    }

    Vec<T>& operator=(const T& other) {
        for (int i = 0; i < _len; ++i) {
            this->data()[i] = other;
        }
        return *this;
    }

    Vec<T>& operator=(Vec<T>&& other) {
        std::swap(_len, other._len);
        std::swap(_capacity, other._capacity);
        std::swap(_data, other._data);
        return *this;
    }

    Vec(Vec<T>&& other) {
        _len = 0;
        _capacity = 0;
        _data = nullptr;
        std::swap(_len, other._len);
        std::swap(_capacity, other._capacity);
        std::swap(_data, other._data);
    }

    int len() const { return _len; }

    T* data() const { return _data; }

    int capacity() const { return _capacity; }

    T* data() { return _data; }

    Vec<T> operator-() {
        T* inv_data_start = new (std::nothrow) T[_len];
        memcpy(inv_data_start, _data, sizeof(T) * _len);
        T* inv_data_end = inv_data_start + _len;
        T* cur = inv_data_start;
        while (cur++ < inv_data_end) *cur = -*cur;
        return Vec(_len, inv_data_start);
    }

#define VEC_OPERATE(opt)                              \
    VEC_ASSERT_TRUE(other.len() == _len);             \
    Vec<T> vec;                                       \
    vec._len = _len;                                  \
    vec._data = new (std::nothrow) T[_len];           \
    for (int i = 0; i < _len; i++) {                  \
        vec.data()[i] = _data[i] opt other.data()[i]; \
    }                                                 \
    return vec;

    template <typename U>
    Vec<T> operator+(const Vec<U>& other) {
        VEC_OPERATE(+)
    }

    template <typename U>
    Vec<T> operator-(const Vec<U>& other) {
        VEC_OPERATE(-)
    }
    template <typename U>
    Vec<T> operator*(const Vec<U>& other) {
        VEC_OPERATE(*)
    }
    template <typename U>
    Vec<T> operator/(const Vec<U>& other) {
        VEC_OPERATE(/)
    }

#define VEC_ASSIGN_OPERATOR(opt)          \
    VEC_ASSERT_TRUE(_len == other.len()); \
    for (int i = 0; i < _len; i++) {      \
        _data[i] opt other.data()[i];     \
    }                                     \
    return *this;

    template <typename U>
    Vec<T>& operator+=(const Vec<U>& other) {
        VEC_ASSIGN_OPERATOR(+=)
    }

    template <typename U>
    Vec<T>& operator-=(const Vec<U>& other) {
        VEC_ASSIGN_OPERATOR(-=)
    }

    template <typename U>
    Vec<T>& operator*=(const Vec<U>& other) {
        VEC_ASSIGN_OPERATOR(*=)
    }

    template <typename U>
    Vec<T>& operator/=(const Vec<U>& other) {
        VEC_ASSIGN_OPERATOR(/=)
    }
#define VEC_ASSIGN_OPERATOR_WITH_CONST(opt) \
    for (int i = 0; i < _len; i++) {        \
        _data[i] opt other;                 \
    }                                       \
    return *this;

    template <typename U>
    Vec<T> operator+=(const U& other) {
        VEC_ASSIGN_OPERATOR_WITH_CONST(+=)
    }

    template <typename U>
    Vec<T> operator-=(const U& other) {
        VEC_ASSIGN_OPERATOR_WITH_CONST(-=)
    }

    template <typename U>
    Vec<T> operator*=(const U& other) {
        VEC_ASSIGN_OPERATOR_WITH_CONST(*=)
    }

    template <typename U>
    Vec<T> operator/=(const U& other) {
        VEC_ASSIGN_OPERATOR_WITH_CONST(/=)
    }

#define VEC_COMPARE(opt)                              \
    VEC_ASSERT_TRUE(_len == other.len());             \
    Vec<R> vec(_len);                                 \
    for (int i = 0; i < _len; i++) {                  \
        vec.data()[i] = _data[i] opt other.data()[i]; \
    }                                                 \
    return vec;

    template <typename U, typename R = bool>
    Vec<R> operator>(const Vec<U>& other) {
        VEC_COMPARE(>);
    }
    template <typename U, typename R = bool>
    Vec<R> operator<(const Vec<U>& other) {
        VEC_COMPARE(<);
    }
    template <typename U, typename R = bool>
    Vec<R> operator==(const Vec<U>& other) {
        VEC_COMPARE(==);
    }
    template <typename U, typename R = bool>
    Vec<R> operator!=(const Vec<U>& other) {
        VEC_COMPARE(!=);
    }
    template <typename U, typename R = bool>
    Vec<R> operator>=(const Vec<U>& other) {
        VEC_COMPARE(>=);
    }
    template <typename U, typename R = bool>
    Vec<R> operator<=(const Vec<U>& other) {
        VEC_COMPARE(<=);
    }

#define VEC_COMPARE_LITERAL(opt)            \
    Vec<R> vec(_len);                       \
    for (int i = 0; i < _len; i++) {        \
        vec.data()[i] = _data[i] opt other; \
    }                                       \
    return vec;

    template <typename U, typename R = bool>
    Vec<R> operator>(const U& other) {
        VEC_COMPARE_LITERAL(>)
    }

    template <typename U, typename R = bool>
    Vec<R> operator<(const U& other) {
        VEC_COMPARE_LITERAL(<)
    }
    template <typename U, typename R = bool>
    Vec<R> operator==(const U& other) {
        VEC_COMPARE_LITERAL(==)
    }
    template <typename U, typename R = bool>
    Vec<R> operator<=(const U& other) {
        VEC_COMPARE_LITERAL(<=)
    }
    template <typename U, typename R = bool>
    Vec<R> operator>=(const U& other) {
        VEC_COMPARE_LITERAL(>=)
    }

    void reserve(int len) {
        VEC_ASSERT_TRUE(len >= 0);
        if (_capacity < len) {
            T* old_data = _data;
            _data = new (std::nothrow) T[len];
            memcpy(_data, old_data, sizeof(T) * _len);
            delete[] old_data;
            _capacity = len;
        }
    }

    const T& operator[](const int index) const {
        VEC_ASSERT_TRUE(index < _len);
        return _data[index];
    }

    T& operator[](const int index) {
        VEC_ASSERT_TRUE(index < _len);
        return _data[index];
    }

    // TODO: optimization here
    Vec<T> operator[](const Vec<bool>& selector) const {
        VEC_ASSERT_TRUE(selector.len() == _len);
        int select_capcity = _len;
        int select_len = 0;
        T* data = new (std::nothrow) T[select_capcity];
        for (int i = 0; i < _len; ++i) {
            if (selector[i]) data[select_len++] = _data[i];
        }
        return Vec<T>(select_capcity, select_len, data);
    }

    void set(const T& value, int index) {
        VEC_ASSERT_TRUE(index < _len);
        _data[index] = value;
    }

    void apply(std::function<T(const T&)>& func) {
        for (int i = 0; i < _len; ++i) {
            _data[i] = func(_data[i]);
        }
    }

    template <typename OP>
    void apply() {
        for (int i = 0; i < _len; ++i) {
            _data[i] = OP::apply(_data[i]);
        }
    }

    template <class U>
    Vec<U> transform(std::function<U(T&)>& func) const {
        Vec<U> vec(_len);
        for (int i = 0; i < _len; ++i) {
            vec.data()[i] = func(_data[i]);
        }
        return vec;
    }

    template <typename OP>
    Vec<T> vec_transform() const {
        Vec<T> vec(_len);
        for (int i = 0; i < _len; ++i) {
            vec.data()[i] = OP::apply(_data[i]);
        }
        return vec;
    }

    template<typename Trans>
    Vec vec_transform(Trans&& trans) {
        Vec<T> vec(_len);
        for (int i = 0; i < _len; ++i) {
            vec[i] = trans(_data[i]);
        }
        return vec;
    }

    void sort() { std::sort(_data, _data + _len); }

    void push_back(const T& t) {
        if (_len == _capacity) {
            reserve(_capacity * 2 + 1);
        }
        _data[_len++] = t;
    }

    void clear() { _len = 0; }

    VecIterator<T> begin() { return VecIterator<T>(_data); }
    VecIterator<T> end() { return VecIterator<T>(_data + _len); }

private:
    int _capacity;
    int _len;
    T* _data;
};
}; // namespace vec
