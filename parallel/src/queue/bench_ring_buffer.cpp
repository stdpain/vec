#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "ring_buffer.h"

constexpr int64_t MAXN = int64_t(1e8);

struct ThreadBind {
    static constexpr auto consumer_id = 1;
    static constexpr auto producer_id = 2;
};

struct SingleThreadBind {
    static constexpr auto consumer_id = 1;
    static constexpr auto producer_id = 1;
};

// benchmark for SPSC
template <class QueueType, class ThreadBindType = ThreadBind, class... Args>
void bench_queue(const char* name, Args&&... args) {
    QueueType queue(std::forward<Args>(args)...);
    auto start = std::chrono::steady_clock::now();
    std::thread pop_thread([&queue]() {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(ThreadBindType::consumer_id, &cpuset);
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
    std::thread push_thread([&queue]() {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(ThreadBindType::producer_id, &cpuset);
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
    std::cerr << name << " costs:" << runtime / 1e6 << "s" << std::endl;
}

int main() {
    // benchmark on Intel(R) Xeon(R) Platinum 8269CY CPU @ 2.50GHz (104 CPUS)
    // 11.3099s
    bench_queue<MutexedRingBuffer<spinlock>>("spin lock buffer", 1023);
    // 23.5687s
    bench_queue<MutexedRingBuffer<std::mutex>>("std::mutex lock buffer", 1023);
    // 2.24, 2.30
    bench_queue<LockFreeRingBufferV1>("LockFreeRingBufferV1", 1023);
    // 1.50, 1.55
    bench_queue<LockFreeRingBufferV2<1024>>("LockFreeRingBufferV2");
    // 1.02, 1.05
    bench_queue<LockFreeRingBufferV3<1024, 60, 60>>("LockFreeRingBufferV3");
    // 0.82, 0.85
    bench_queue<LockFreeRingBufferV3<1024, 60, 120>>("LockFreeRingBufferV4");
    // 0.815
    bench_queue<LockFreeRingBufferV3<1024, 60, 120>, SingleThreadBind>("single thread");

    return 0;
}