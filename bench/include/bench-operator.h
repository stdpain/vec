#pragma once

#include <memory>
#include <vector>

#include "bench-state.h"
#include "status.h"

namespace stdpain {
class BenchOperator;
using BenchOperatorPtr = std::shared_ptr<BenchOperator>;
class BenchOperator : public std::enable_shared_from_this<BenchOperator> {
public:
    Status init();
    Status prepare();
    Status open();
    Status close();

private:
    std::weak_ptr<BenchOperator> _parent;
    std::vector<BenchOperatorPtr> _children;
};

} // namespace stdpain