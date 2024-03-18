#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "design_pattern/spk_singleton.hpp"

using namespace spk;

// A test class that will use the Singleton base class
class TestSingleton : public Singleton<TestSingleton> {
    friend class Singleton<TestSingleton>; // Allow Singleton to access the constructor

private:
    TestSingleton() = default; // Private constructor

public:
    int value = 0;

    void setValue(int newValue) {
        value = newValue;
    }

    int getValue() const {
        return value;
    }
};

class SingletonTest : public ::testing::Test {
protected:
    virtual void SetUp() override {
        // Ensure the singleton is released before each test
        TestSingleton::release();
    }

    virtual void TearDown() override {
        // Clean up after each test
        TestSingleton::release();
    }
};

TEST_F(SingletonTest, CreateAndAccessInstance) {
    auto& instance = TestSingleton::instanciate();
    instance->setValue(42);
    ASSERT_EQ(instance->getValue(), 42);
}

TEST_F(SingletonTest, DoubleInstantiation) {
    TestSingleton::instanciate();
    EXPECT_THROW(TestSingleton::instanciate(), std::runtime_error);
}

TEST_F(SingletonTest, AccessInstance) {
    auto& instance1 = TestSingleton::instanciate();
    instance1->setValue(42);

    auto& instance2 = TestSingleton::instance();
    ASSERT_EQ(instance2->getValue(), 42);
}

TEST_F(SingletonTest, ReleaseInstance) {
    auto& instance = TestSingleton::instanciate();
    instance->setValue(42);

    TestSingleton::release();

    EXPECT_TRUE(TestSingleton::instance() == nullptr);
}
