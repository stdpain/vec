
#include <malloc.h>
#include <atomic>

void* plt_call() {
    static std::atomic_int v{};
    v++;
    return nullptr;
}