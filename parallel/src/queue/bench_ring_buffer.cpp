#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "ring_buffer.h"
constexpr int64_t MAXN = int64_t(1e8);
// benchmark on Intel(R) Xeon(R) Platinum 8269CY CPU @ 2.50GHz (104 CPUS)
MutexedRingBuffer<spinlock> queue(1023); // 11.3099s 
// MutexedRingBuffer<std::mutex> queue(1023); // 23.5687s
// LockFreeRingBufferV1 queue(1023); // 2.24, 2.30
// LockFreeRingBufferV2<1024> queue; // 1.50, 1.55
// LockFreeRingBufferV3<1024, 60, 60> queue; // 1.02, 1.05
// LockFreeRingBufferV3<1024, 60, 120> queue; // 0.82, 0.85

int main() {
    auto start = std::chrono::steady_clock::now();
    std::thread pop_thread([]() {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(1, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        volatile int64_t cnt = 0, last = 0;
        while (cnt != MAXN) {
            size_t value;
            if (queue.empty()) {
                std::this_thread::yield();
            }
            if (queue.try_pop(&value)) {
                cnt += 1;
                assert(value > last);
                last = value;
            }
        }
    });
    std::thread push_thread([]() {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(2, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        for (int64_t i = 1; i <= MAXN;) {
            if (queue.is_full()) {
                std::this_thread::yield();
            }
            if (queue.try_push(i)) {
                i += 1;
            }
        }
    });
    push_thread.join();
    pop_thread.join();
    auto finish = std::chrono::steady_clock::now();
    auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    std::cerr << runtime / 1e6 << "s" << std::endl;
    return 0;
}