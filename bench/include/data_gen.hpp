#pragma once

#include <memory>

#include "types.hpp"

namespace stdpain {
template <class T>
struct DefaultValueGenerator {
    T next_value() { return T(); }
};

template <class T>
struct AlwaysZeroGenerator {
    T next_value() { return 0; }
};

template <class T>
struct AlwaysOneGenerator {
    T next_value() { return 1; }
};

template <class DataGenerator, class Container>
class ContainerIniter {
public:
    ContainerIniter(std::unique_ptr<DataGenerator>&& generator)
            : _generator(std::move(generator)) {}
    void init_container(Container& container, int size) {
        container.resize(size);
        for (int i = 0; i < container.size(); ++i) {
            container[i] =_generator->next_value();
        }
    }

private:
    std::unique_ptr<DataGenerator> _generator;
};
} // namespace stdpain