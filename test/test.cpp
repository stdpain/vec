#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <string>

#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "vec/vec.h"
#include "vec/vec_string.h"

namespace vec {
TEST(VecBasicTest, Test) {
    const int test_vec_length = 1024;
    Vec<int> ivec(test_vec_length);
    // test ivec length
    ASSERT_EQ(ivec.len(), test_vec_length);
    // test vec add and default value
    //expect vectorized
    ivec += 4;
    for (int i = 0; i < test_vec_length; ++i) {
        ASSERT_EQ(ivec.data()[i], 4);
    }
    Vec<double> dvec(test_vec_length);
    // test assign function
    dvec = 4.5;
    for (int i = 0; i < test_vec_length; i++) {
        ASSERT_EQ(dvec.data()[i], 4.5);
    }
    // test add function
    dvec += ivec;
    for (int i = 0; i < dvec.len(); i++) {
        ASSERT_EQ(dvec.data()[i], 4 + 4.5);
    }
    auto bvec = dvec > ivec;
    for (int i = 0; i < dvec.len(); i++) {
        ASSERT_TRUE(bvec.data()[i]);
    }
    //expect vectorized
    int counter = 0;
    for (int i = 0; i < dvec.len(); i += 10) {
        dvec[i] *= 1024;
        counter++;
    }
    //no vectorized
    //expect vectorized not not
    auto bvec2 = dvec > 1024;
    Vec<double> dvec2 = dvec[bvec2];
    ASSERT_EQ(dvec2.len(), counter);

    // bench start
    // generate data
    std::default_random_engine e;
    std::uniform_int_distribution<unsigned int> u(0, 9);
    std::function<int(double&)> func = [&](double& x) { return u(e); };
    Vec<int> ivec2 = dvec2.transform(func);
    ivec2.sort();

    struct SquareFunctionImpl {
        using ResultType = int;
        static inline ResultType apply(int& i) { return i * i; }
    };
    //expect vectorized
    ivec2.vec_transform<SquareFunctionImpl>();
}

TEST(VecBasicTest, IteratorTest) {
    std::default_random_engine e;
    std::uniform_int_distribution<int> u(0, 4096);
    std::function<int(int&)> func = [&](int& x) { return u(e); };
    Vec<int> zero(4096);
    Vec<int> ivec = zero.transform(func);

    // test iterator
    std::count_if(ivec.begin(), ivec.end(), [](auto ele) { return ele > 0; });
    ASSERT_LT(ivec.begin(), ivec.end());
    std::sort(ivec.begin(), ivec.end());
    int last = std::numeric_limits<int>::min();
    for (auto v : ivec) {
        ASSERT_LE(last, v);
        last = v;
    }
    Vec<int> ivec2 = zero.transform(func);
    int counter = 0;
    //no vectorized
    for (auto v : ivec2) {
        counter += v;
    }
    int counter2 = 0;
    //no vectorized
    for (int i = 0; i < ivec2.len(); i++) {
        counter2 += ivec2.data()[i];
    }
    ASSERT_EQ(counter, counter2);
}

TEST(VecBasicTest, FixedStringTest) {
    const char* test_text = "Hello World";
    char tester[4096];
    strcpy(tester, test_text);
    tester[strlen(test_text)] = '!';
    LOG(INFO) << "sizeof(test_text) " << sizeof(test_text);
    ASSERT_EQ(strncmp(tester, test_text, strlen(test_text)), 0);

    Vec<FixedString> vec(11, 1024);
    vec = test_text;
    for (int i = 0; i < vec.len(); i++) {
        strncmp(vec[i], test_text, sizeof(test_text));
    }

    Vec<FixedString> vec2(11, 1024);
    vec2 = std::move(vec);
    std::string str = "test";
    vec2.set(str, 100);
    ASSERT_EQ(strncmp(vec2[100], str.c_str(), str.length()), 0);
}
} // namespace vec

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
