#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "vec/vec.h"
#include "vec/vec_iterator.h"

namespace vec {

template <>
class Vec<std::string> {
public:
    Vec(int size) {
        _data.resize(size);
    }

    ~Vec() = default;

    Vec(const Vec& other) {
        _data = other._data;
    }

    Vec& operator=(const Vec& other) {
        _data = other._data;
        return *this;
    }

    Vec& operator=(const std::string& str) {
        for(int i = 0;i < _data.size(); ++i) {
            _data[i] = str;
        }
        return *this;
    }

    Vec& operator=(Vec&& other) {
        std::swap(this->_data, other._data);
        return *this;
    }

    Vec(Vec&& other) {
        std::swap(this->_data, other._data);
    }

    int len() const { return _data.size(); }

    int capacity() const { return _data.capacity(); }

    Vec<int> strlen() const {
        int size = _data.size();
        Vec<int> len_vec(_data.size());
        for(int i = 0;i < size; ++i) {
            len_vec[i] = _data[i].size();
        }
        return len_vec;
    };

    const std::string& operator[](const int index) const {
        return this->at(index);
    }

    std::string& operator[](const int index) {
        return this->at(index);
    }

    const std::string& at(const int index) const {
        return _data[index];
    }

    std::string& at(const int index) {
        return _data[index];
    }

    // support std::string_view, std::string
    template <typename StringType>
    void set(const StringType& value, int index) {
        _data[index] = value;
    }

    void reserve(int len) {
        VEC_ASSERT_TRUE(len >= 0);
        _data.reserve(len);
    }

    void apply(std::function<void(std::string&)>& func) {
        for (int i = 0; i < len(); ++i) {
            func(this->at(i));
        }
    }

    template <typename OP>
    void apply() {
        for (int i = 0; i < len(); ++i) {
            OP::apply(this->at(i));
        }
    }

    template <class U>
    Vec<U> transform(std::function<void(const std::string&, U&, int)>& func) const {
        Vec<U> vec(len());
        for (int i = 0; i < len(); ++i) {
            func(this->at(i), vec.at(i));
        }
        return vec;
    }

    template <class StringType>
    void push_back(const StringType& t) {
        _data.emplace_back(t);
    }

    void clear() { _data.clear(); }

    // TODO: compare, function, iterator

private:
    std::vector<std::string> _data;
};

} // namespace vec
