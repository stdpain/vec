#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>

class RingBuffer {
public:
    RingBuffer(int capacity) : _capacity(capacity) { _datas = std::make_unique<int[]>(capacity); }
    bool is_full() const { return _size == _capacity; }
    bool empty() const { return _size == 0; }
    int size() const { return _size; }
    void push(int val) {
        assert(!is_full());
        _datas[_end] = val;
        _end++;
        _end = _end % _capacity;
        _size++;
    }
    int pop_front() {
        assert(empty());
        int ret = _datas[_start];
        _start++;
        _start %= _capacity;
        _size--;
        return ret;
    }

private:
    int _start{};
    int _end{};
    int _size{};
    const int _capacity;
    std::unique_ptr<int[]> _datas;
};

// without extra size
class RingBufferV2 {
public:
    // real capacify == V1 - 1
    RingBufferV2(int capacity) : _capacity(capacity + 1) {
        _datas = std::make_unique<int[]>(capacity);
    }
    bool is_full() const { return _end + 1 % _capacity == _start; }
    bool empty() const { return _end == _start; }

    void push(int val) {
        assert(!is_full());
        _datas[_end] = val;
        _end++;
        _end = _end % _capacity;
    }
    int pop_front() {
        assert(empty());
        int ret = _datas[_start];
        _start++;
        _start %= _capacity;
        return ret;
    }

private:
    int _start{};
    int _end{};
    const int _capacity;
    std::unique_ptr<int[]> _datas;
};

// implements in https://rigtorp.se/spinlock/
struct spinlock {
    std::atomic<bool> lock_ = {0};

    void lock() noexcept {
        for (;;) {
            // Optimistically assume the lock is free on the first try
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                return;
            }
            // Wait for lock to be released without generating cache misses
            while (lock_.load(std::memory_order_relaxed)) {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
                // hyper-threads
                __builtin_ia32_pause();
            }
        }
    }

    bool try_lock() noexcept {
        // First do a relaxed load to check if lock is free in order to prevent
        // unnecessary cache misses if someone does while(!try_lock())
        return !lock_.load(std::memory_order_relaxed) &&
               !lock_.exchange(true, std::memory_order_acquire);
    }

    void unlock() noexcept { lock_.store(false, std::memory_order_release); }
};

// mutexed ring buffer
template <class Mutex>
class MutexedRingBuffer {
public:
    MutexedRingBuffer(int capacity) : _capacity(capacity + 1) {
        assert((_capacity & (_capacity - 1)) == 0);
        _datas = std::make_unique<size_t[]>(_capacity);
    }
    bool empty() const {
        // std::lock_guard guard(_mutex);
        return _start == _end;
    }
    bool is_full() const {
        // std::lock_guard guard(_mutex);
        return (_end + 1 & (_capacity - 1)) == _start;
    }

    bool try_push(size_t val) {
        if (is_full()) {
            return false;
        }
        std::lock_guard guard(_mutex);
        _datas[_end] = val;
        _end++;
        _end &= _capacity - 1;
        return true;
    }

    bool try_pop(size_t* val) {
        if (empty()) {
            return false;
        }
        std::lock_guard guard(_mutex);
        *val = _datas[_start];
        _start++;
        _start &= _capacity - 1;
        return true;
    }

private:
    mutable Mutex _mutex;
    int32_t _start{};
    int32_t _end{};
    const int _capacity;
    std::unique_ptr<size_t[]> _datas;
};

// lock-free SPSC
class LockFreeRingBufferV1 {
public:
    LockFreeRingBufferV1(int capacity) : _capacity(capacity + 1) {
        assert((_capacity & (_capacity - 1)) == 0);
        _datas = std::make_unique<size_t[]>(_capacity);
    }
    bool empty() const {
        // return _start.load(std::memory_order_acquire) == _end.load(std::memory_order_acquire);
        return _start.load(std::memory_order_relaxed) == _end.load(std::memory_order_relaxed);
    }
    bool is_full() const {
        // int next = _end.load(std::memory_order_acquire) + 1;
        int next = _end.load(std::memory_order_relaxed) + 1;
        // return (next & (_capacity - 1)) == _start.load(std::memory_order_acquire);
        return (next & (_capacity - 1)) == _start.load(std::memory_order_relaxed);
    }

    bool try_push(size_t val) {
        if (is_full()) {
            return false;
        }
        int offset = _end.load(std::memory_order_acquire);
        _datas[offset] = val;
        offset++;
        offset &= _capacity - 1;
        _end.store(offset, std::memory_order_release);
        return true;
    }

    bool try_pop(size_t* val) {
        if (empty()) {
            return false;
        }
        int prev = _start.load(std::memory_order_acquire);
        *val = _datas[prev];
        prev++;
        prev &= _capacity - 1;
        _start.store(prev, std::memory_order_release);
        return true;
    }

private:
    std::atomic<int32_t> _start{};
    std::atomic<int32_t> _end{};
    const int _capacity;
    std::unique_ptr<size_t[]> _datas;
};

template <size_t capacity>
class LockFreeRingBufferV2 {
public:
    LockFreeRingBufferV2() {
        static_assert((capacity & (capacity - 1)) == 0);
        _datas = std::make_unique<size_t[]>(capacity);
    }
    bool empty() const {
        return _start.load(std::memory_order_relaxed) == _end.load(std::memory_order_relaxed);
    }
    bool is_full() const {
        int next = _end.load(std::memory_order_relaxed) + 1;
        return (next & (capacity - 1)) == _start.load(std::memory_order_relaxed);
    }

    bool try_push(size_t val) {
        if (is_full()) {
            return false;
        }
        int offset = _end.load(std::memory_order_acquire);
        _datas[offset] = val;
        offset++;
        offset &= capacity - 1;
        _end.store(offset, std::memory_order_release);
        return true;
    }

    bool try_pop(size_t* val) {
        if (empty()) {
            return false;
        }
        int prev = _start.load(std::memory_order_acquire);
        *val = _datas[prev];
        prev++;
        prev &= capacity - 1;
        _start.store(prev, std::memory_order_release);
        return true;
    }

private:
    std::atomic<int32_t> _start{};
    // size_t pad1;
    // int32_t longpad1[100];
    // char padding1[60];
    std::atomic<int32_t> _end{};
    // char padding2[60];
    // int32_t longpad2[100];
    // size_t pad2;
    // char padding3[60];
    std::unique_ptr<size_t[]> _datas;
};

template <size_t capacity, size_t len_padding1, size_t len_padding2>
class LockFreeRingBufferV3 {
public:
    LockFreeRingBufferV3() {
        static_assert((capacity & (capacity - 1)) == 0);
        _datas = std::make_unique<size_t[]>(capacity);
    }
    bool empty() const {
        return _start.load(std::memory_order_relaxed) == _end.load(std::memory_order_relaxed);
    }
    bool is_full() const {
        int next = _end.load(std::memory_order_relaxed) + 1;
        return (next & (capacity - 1)) == _start.load(std::memory_order_relaxed);
    }

    bool try_push(size_t val) {
        if (is_full()) {
            return false;
        }
        int offset = _end.load(std::memory_order_acquire);
        _datas[offset] = val;
        offset++;
        offset &= capacity - 1;
        _end.store(offset, std::memory_order_release);
        return true;
    }

    bool try_pop(size_t* val) {
        if (empty()) {
            return false;
        }
        int prev = _start.load(std::memory_order_acquire);
        *val = _datas[prev];
        prev++;
        prev &= capacity - 1;
        _start.store(prev, std::memory_order_release);
        return true;
    }

private:
    std::atomic<int32_t> _start{};
    char padding1[len_padding1];
    std::atomic<int32_t> _end{};
    char padding2[len_padding2];
    std::unique_ptr<size_t[]> _datas;
};
