#pragma once
#include <initializer_list>
#include "types.hpp"

namespace stdpain {
class BenchMarkDecriptor;

class ChunkDescriptor {
public:
    ChunkDescriptor(RuntimeDataTypes&& types) : _data_types(std::move(types)) {}
    const auto& data_types() { return _data_types; }

private:
    RuntimeDataTypes _data_types;
};

class ChunkDescriptorBuilder {
public:
    ChunkDescriptorBuilder& append(PrimitiveType type) {
        _types.emplace_back(RuntimePrimitiveTypeFactory::get_primitive_type(type));
        return *this;
    }

    ChunkDescriptorBuilder& append(std::initializer_list<PrimitiveType> ptypes) {
        for(const auto ptype: ptypes) {
            auto type = RuntimePrimitiveTypeFactory::get_primitive_type(ptype);
            _types.emplace_back(std::move(type));
        }
        return *this;
    }

    ChunkDescriptor build() { return ChunkDescriptor(std::move(_types)); }

private:
    RuntimeDataTypes _types;
};

class BenchMarkDescriptor {
public:
};

class BenchMarkDesriptorBuilder {
public:
    
};

} // namespace stdpain