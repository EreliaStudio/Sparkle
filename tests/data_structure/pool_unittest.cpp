#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "data_structure/spk_pool.hpp"

using namespace spk;

template<typename T>
class SimpleObject
{
public:
    T value;
    SimpleObject(T val) : value(val) {}
    void reset(T newVal) { value = newVal; }
};

class PoolTest : public ::testing::Test
{
protected:
    Pool<SimpleObject<int>> intPool;
    Pool<SimpleObject<std::string>> stringPool;

    PoolTest() 
        : intPool([]() { return new SimpleObject<int>(42); }, [](SimpleObject<int>& obj) { obj.reset(0); }),
          stringPool([]() { return new SimpleObject<std::string>("AllocatedString"); }, [](SimpleObject<std::string>& obj) { obj.reset("initial"); }) {}

    virtual void SetUp() override
    {

    }
};

TEST_F(PoolTest, Constructor_Default)
{
    ASSERT_EQ(intPool.size(), 0);
}

TEST_F(PoolTest, EditCleaner)
{
    stringPool.editCleaner([](SimpleObject<std::string>& obj) { obj.reset("CustomCleanerString"); });
    auto obj = stringPool.obtain();
    ASSERT_EQ(obj->value, "CustomCleanerString");
}

TEST_F(PoolTest, Allocate)
{
    intPool.allocate();
    ASSERT_EQ(intPool.size(), 1);
}

TEST_F(PoolTest, Resize)
{
    intPool.resize(5);
    ASSERT_EQ(intPool.size(), 5);
}

TEST_F(PoolTest, Obtain)
{
    auto obj = stringPool.obtain();
    ASSERT_EQ(obj->value, "initial");
    ASSERT_EQ(stringPool.size(), 0);
}

TEST_F(PoolTest, CleanerFunctionality)
{
    auto obj = intPool.obtain();
    ASSERT_EQ(obj->value, 0);
    obj->reset(100);
}

TEST_F(PoolTest, ReturnToPoolWithCleanup)
{
    {
        auto obj = intPool.obtain();
        obj->reset(100);
    }

    ASSERT_EQ(intPool.size(), 1);
	
    auto newObj = intPool.obtain();
    ASSERT_EQ(newObj->value, 0);
}