#pragma once

#include <string_view>
namespace stdpain {

inline size_t remove_tailing_zero(std::string_view view) {
    int64_t size = view.size();
    const char* data = view.data();
    while (size > 0 && data[size - 1] == '\0') size--;
    return size;
}

} // namespace stdpain
