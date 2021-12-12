// design similar to
// https://github.com/StarRocks/starrocks/blob/main/be/src/runtime/primitive_type.h
// https://github.com/StarRocks/starrocks/blob/main/be/src/column/type_traits.h

#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace stdpain {
// Primitive TYPE
enum class PrimitiveType {
    INVALID_TYPE,
    TYPE_NULL,
    TYPE_BOOLEAN,
    TYPE_TINYINT,
    TYPE_SMALLINT,
    TYPE_INT,
    TYPE_BIGINT,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_FIXED_STRING,
};

template <PrimitiveType primitive_type>
struct RunTimeTypeTraits {};
template <>
struct RunTimeTypeTraits<PrimitiveType::TYPE_NULL> {
    using CppType = uint8_t;
};

template <>
struct RunTimeTypeTraits<PrimitiveType::TYPE_BOOLEAN> {
    using CppType = uint8_t;
};

template <>
struct RunTimeTypeTraits<PrimitiveType::TYPE_TINYINT> {
    using CppType = int8_t;
};

template <>
struct RunTimeTypeTraits<PrimitiveType::TYPE_SMALLINT> {
    using CppType = int16_t;
};

template <>
struct RunTimeTypeTraits<PrimitiveType::TYPE_INT> {
    using CppType = int32_t;
};

template <>
struct RunTimeTypeTraits<PrimitiveType::TYPE_BIGINT> {
    using CppType = int64_t;
};

template <>
struct RunTimeTypeTraits<PrimitiveType::TYPE_FLOAT> {
    using CppType = float;
};

template <>
struct RunTimeTypeTraits<PrimitiveType::TYPE_DOUBLE> {
    using CppType = double;
};

class IRuntimeDataType {
public:
    virtual bool is_primitive_type() { return true; }
};

struct RuntimePrimitiveType : public IRuntimeDataType {
public:
    virtual bool is_primitive_type() { return true; }
    virtual PrimitiveType primitive_type() { return PrimitiveType::INVALID_TYPE; }
};

template <PrimitiveType TYPE>
struct RuntimePrimitiveTypeImpl : public RuntimePrimitiveType {
    PrimitiveType primitive_type() override { return TYPE; }
};

using RuntimeDataTypePtr = std::shared_ptr<IRuntimeDataType>;
using RuntimeDataTypes = std::vector<RuntimeDataTypePtr>;

class RuntimePrimitiveTypeFactory {
public:
    static RuntimeDataTypePtr get_primitive_type(PrimitiveType type);
};

#define APPLY_FOR_TYPE_INTEGER(M)   \
    M(PrimitiveType::TYPE_TINYINT)  \
    M(PrimitiveType::TYPE_SMALLINT) \
    M(PrimitiveType::TYPE_INT)      \
    M(PrimitiveType::TYPE_BIGINT)

#define APPLY_FOR_TYPE_FLOAT(M)  \
    M(PrimitiveType::TYPE_FLOAT) \
    M(PrimitiveType::TYPE_DOUBLE)

#define APPLY_FOR_TYPE_NUMBER(M) \
    APPLY_FOR_TYPE_INTEGER(M)    \
    APPLY_FOR_TYPE_FLOAT(M)

#define APPLY_FOR_TYPE_STRING(M) M(PrimitiveType::TYPE_FIXED_STRING)

#define APPLY_FOR_ALL_PRIM_TYPES(M) \
    M(PrimitiveType::TYPE_NULL)     \
    APPLY_FOR_TYPE_NUMBER(M)        \
    APPLY_FOR_TYPE_STRING(M)

} // namespace stdpain