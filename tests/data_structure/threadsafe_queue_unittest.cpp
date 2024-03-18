#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "data_structure/spk_thread_safe_queue.hpp"

using namespace spk;

class ThreadSafeDequeTest : public ::testing::Test {
protected:
    ThreadSafeDeque<int> intDeque;

    virtual void SetUp() override {
        // Initialize deque if necessary
    }
};

TEST_F(ThreadSafeDequeTest, IsEmptyOnConstruction) {
    ASSERT_TRUE(intDeque.empty());
}

TEST_F(ThreadSafeDequeTest, PushBackPopFront) {
    intDeque.push_back(1);
    ASSERT_EQ(intDeque.pop_front(), 1);
    ASSERT_TRUE(intDeque.empty());
}

TEST_F(ThreadSafeDequeTest, PushFrontPopBack) {
    intDeque.push_front(1);
    ASSERT_EQ(intDeque.pop_back(), 1);
    ASSERT_TRUE(intDeque.empty());
}

TEST_F(ThreadSafeDequeTest, SizeAfterMultiplePushes) {
    intDeque.push_back(1);
    intDeque.push_back(2);
    intDeque.push_front(3);
    ASSERT_EQ(intDeque.size(), 3);
}

TEST_F(ThreadSafeDequeTest, AccessFrontAndBack) {
    intDeque.push_back(1);
    intDeque.push_back(2);
    intDeque.push_front(3);
    ASSERT_EQ(intDeque.front(), 3);
    ASSERT_EQ(intDeque.back(), 2);
}

TEST_F(ThreadSafeDequeTest, ClearDeque) {
    intDeque.push_back(1);
    intDeque.push_back(2);
    intDeque.clear();
    ASSERT_TRUE(intDeque.empty());
}

void producer(ThreadSafeDeque<int>& deque) {
    for (int i = 0; i < 10; ++i) {
        deque.push_back(int(i));
    }
}

void consumer(ThreadSafeDeque<int>& deque, std::vector<int>& consumed) {
    for (int i = 0; i < 10; ++i) {
        deque.wait();
        consumed.push_back(deque.pop_front());
    }
}

TEST_F(ThreadSafeDequeTest, ThreadSafety) {
    std::vector<int> consumed;
    std::thread prod(producer, std::ref(intDeque));
    std::thread cons(consumer, std::ref(intDeque), std::ref(consumed));

    prod.join();
    cons.join();

    ASSERT_EQ(consumed.size(), 10);
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(consumed[i], i);
    }
}