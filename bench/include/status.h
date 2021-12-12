#pragma once

namespace stdpain {

class Status {
public:
    Status() = default;
    ~Status() noexcept = default;
    bool ok() const { return _err_code == 0; }
    inline operator bool() const { return ok(); }

private:
    int _err_code = 0;
};
} // namespace stdpain

// some generally useful macros
#define RETURN_IF_ERROR(stmt)            \
    do {                                 \
        const Status& _status_ = (stmt); \
        if (UNLIKELY(!_status_.ok())) {  \
            return _status_;             \
        }                                \
    } while (false)
