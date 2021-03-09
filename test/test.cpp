#include <functional>
#include <iostream>
#include <random>
#include <string>

#include "vec/vec.h"

int main() {
    using namespace vec;
    Vec<int> ivec(1024);
    //expect vectorized
    ivec += 4;
    Vec<double> dvec(1024);
    //expect vectorized
    dvec += 4.5;
    //expect vectorized
    dvec += ivec;
    for (int i = 0; i < dvec.len(); i++) {
        assert(dvec.data()[i] == 4 + 4.5);
    }
    auto bvec = dvec > ivec;
    for (int i = 0; i < dvec.len(); i++) {
        assert(bvec.data()[i]);
    }
    int counter = 0;
    for (int i = 0; i < dvec.len(); i += 10) {
        dvec[i] *= 1024;
        counter++;
    }
    //expect vectorized
    auto bvec2 = dvec > 1024;
    Vec<double> dvec2 = dvec[bvec2];
    assert(dvec2.len() == counter);

    // bench start
    // generate data
    std::default_random_engine e;
    std::uniform_int_distribution<unsigned int> u(0, 9);
    std::function<int(double&)> func = [&](double& x) { return u(e); };
    Vec<int> ivec2 = dvec2.transform(func);
    ivec2.sort();

    struct SquareFunctionImpl {
        using ResultType = int;
        static inline ResultType apply(int &i) { return i*i; }
    };
    ivec2.vec_transform<SquareFunctionImpl>();

    return 0;
}
