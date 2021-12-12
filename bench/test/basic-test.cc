#include "bench-descriptor.h"
#include "gtest/gtest.h"

namespace stdpain {
namespace test {
TEST(BENCH_INTERNAL_TEST, TypeTest) {
    auto data_type_int = RuntimePrimitiveTypeFactory::get_primitive_type(PrimitiveType::TYPE_INT);
}
} // namespace test
} // namespace stdpain

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}