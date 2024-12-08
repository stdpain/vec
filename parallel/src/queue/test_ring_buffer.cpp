#include <queue>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "ring_buffer.h"

std::default_random_engine gen;
std::uniform_int_distribution<int> d(0, 4096);

TEST(RingBufferTest, insert) {
    RingBuffer buffer(5);
    for (int j = 0; j < 10; ++j) {
        for (int i = 0; i < 5; ++i) {
            buffer.push(i);
        }
        ASSERT_TRUE(buffer.is_full());
        for (int i = 0; i < 5; ++i) {
            int res = buffer.pop_front();
            ASSERT_TRUE(res == i);
        }
        ASSERT_TRUE(buffer.empty());
    }
    size_t sz = 0;
    const size_t num = 4096;
    std::queue<int> q;
    // OP: insert/pop
    for (int j = 0; j < num; ++j) {
        int op = d(gen) % 2;
        if (op == 0 && sz > 0) {
            int v = buffer.pop_front();
            ASSERT_EQ(v, q.front());
            q.pop();
        } else if (op == 1) {
            buffer.push(j);
            q.push(j);
        }
    }
}
