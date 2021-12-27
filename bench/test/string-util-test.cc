#include <cstring>
#include <string_view>

#include "gtest/gtest.h"
#include "string_utils.hpp"

namespace stdpain {
TEST(BENCH_INTERNAL_STRING, RemoveTailingZero) {
    {
        char char_with_tail[20] = "Hello";
        size_t slen = sizeof(char_with_tail);
        size_t len1 = strnlen(char_with_tail, slen);
        size_t len2 = remove_tailing_zero(std::string_view(char_with_tail, slen));
        ASSERT_EQ(len1, len2);
    }
    {
        char empty_str[1] = "";
        size_t slen = sizeof(empty_str);
        size_t len1 = strnlen(empty_str, slen);
        size_t len2 = remove_tailing_zero(std::string_view(empty_str, slen));
        ASSERT_EQ(len1, len2);
    }

}
} // namespace stdpain
