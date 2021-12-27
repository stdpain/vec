#pragma once

#include <limits>
#include <set>

namespace stdpain {

template <class T>
struct Range {
    bool inc_begin = true;
    bool inc_end = true;
    T begin_key = std::numeric_limits<T>::lowest();
    T end_key = std::numeric_limits<T>::max();
};

template <class T>
struct FixedValues {
    std::set<T> values;
};

template<class T>
class RangeValues {
public:

private:
    Range<T> _range;
    FixedValues<T> _values;
};

} // namespace stdpain
