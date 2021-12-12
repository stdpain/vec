#include "types.hpp"

#include <unordered_map>

namespace stdpain {

class PrimitiveDataTypeMap {
public:
    PrimitiveDataTypeMap() {
#define M(PTYPE) _datas[PTYPE] = std::make_shared<RuntimePrimitiveTypeImpl<PTYPE>>();
    APPLY_FOR_ALL_PRIM_TYPES(M)
#undef M
    }
    RuntimeDataTypePtr operator[](PrimitiveType type) { return _datas[type]; }

private:
    std::unordered_map<PrimitiveType, RuntimeDataTypePtr> _datas;
};

RuntimeDataTypePtr RuntimePrimitiveTypeFactory::get_primitive_type(PrimitiveType type) {
    static PrimitiveDataTypeMap s_data_type_map;
    return s_data_type_map[type];
}
} // namespace stdpain