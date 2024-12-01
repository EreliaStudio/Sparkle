#pragma once

#include "gtest/gtest.h"

#include "structure/container/spk_pool.hpp"

class PoolTest : public ::testing::Test
{
protected:
    struct TestObject
    {
        int value;
        TestObject() : value(0) {}
    };

    using TestPool = spk::Pool<TestObject>;

    TestPool::Allocator allocator = []() { return new TestObject(); };
    TestPool::Cleaner cleaner = [](TestObject& obj) { obj.value = 0; };
    TestPool pool{ allocator, cleaner };
};