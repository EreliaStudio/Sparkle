#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "math/spk_quaternion.hpp"

using namespace spk;

class QuaternionOperatorTest : public ::testing::Test {
protected:
    Quaternion q1;
    Quaternion q2;

    virtual void SetUp() override {
        // Initializing two quaternions for testing
        q1 = Quaternion(1.0f, 2.0f, 3.0f, 4.0f);
        q2 = Quaternion(5.0f, 6.0f, 7.0f, 8.0f);
    }
};

TEST_F(QuaternionOperatorTest, Addition) {
    Quaternion result = q1 + q2;
    ASSERT_FLOAT_EQ(result.x, 6.0f);
    ASSERT_FLOAT_EQ(result.y, 8.0f);
    ASSERT_FLOAT_EQ(result.z, 10.0f);
    ASSERT_FLOAT_EQ(result.w, 12.0f);
}

TEST_F(QuaternionOperatorTest, Subtraction) {
    Quaternion result = q1 - q2;
    ASSERT_FLOAT_EQ(result.x, -4.0f);
    ASSERT_FLOAT_EQ(result.y, -4.0f);
    ASSERT_FLOAT_EQ(result.z, -4.0f);
    ASSERT_FLOAT_EQ(result.w, -4.0f);
}

TEST_F(QuaternionOperatorTest, ScalarMultiplication) {
    Quaternion result = q1 * 2.0f;
    ASSERT_FLOAT_EQ(result.x, 2.0f);
    ASSERT_FLOAT_EQ(result.y, 4.0f);
    ASSERT_FLOAT_EQ(result.z, 6.0f);
    ASSERT_FLOAT_EQ(result.w, 8.0f);
}

TEST_F(QuaternionOperatorTest, ScalarDivision) {
    Quaternion result = q1 / 2.0f;
    ASSERT_FLOAT_EQ(result.x, 0.5f);
    ASSERT_FLOAT_EQ(result.y, 1.0f);
    ASSERT_FLOAT_EQ(result.z, 1.5f);
    ASSERT_FLOAT_EQ(result.w, 2.0f);
}

TEST_F(QuaternionOperatorTest, CompoundAddition) {
    q1 += q2;
    ASSERT_FLOAT_EQ(q1.x, 6.0f);
    ASSERT_FLOAT_EQ(q1.y, 8.0f);
    ASSERT_FLOAT_EQ(q1.z, 10.0f);
    ASSERT_FLOAT_EQ(q1.w, 12.0f);
}

TEST_F(QuaternionOperatorTest, CompoundSubtraction) {
    q1 -= q2;
    ASSERT_FLOAT_EQ(q1.x, -4.0f);
    ASSERT_FLOAT_EQ(q1.y, -4.0f);
    ASSERT_FLOAT_EQ(q1.z, -4.0f);
    ASSERT_FLOAT_EQ(q1.w, -4.0f);
}

TEST_F(QuaternionOperatorTest, CompoundScalarMultiplication) {
    q1 *= 2.0f;
    ASSERT_FLOAT_EQ(q1.x, 2.0f);
    ASSERT_FLOAT_EQ(q1.y, 4.0f);
    ASSERT_FLOAT_EQ(q1.z, 6.0f);
    ASSERT_FLOAT_EQ(q1.w, 8.0f);
}

TEST_F(QuaternionOperatorTest, CompoundScalarDivision) {
    q1 /= 2.0f;
    ASSERT_FLOAT_EQ(q1.x, 0.5f);
    ASSERT_FLOAT_EQ(q1.y, 1.0f);
    ASSERT_FLOAT_EQ(q1.z, 1.5f);
    ASSERT_FLOAT_EQ(q1.w, 2.0f);
}

// This function could be used to test scalar multiplication from the right-hand side
TEST_F(QuaternionOperatorTest, ScalarMultiplicationCommutes) {
    Quaternion result1 = 2.0f * q1;
    Quaternion result2 = q1 * 2.0f;
    ASSERT_TRUE(result1 == result2);
}

// And similarly for division if that makes sense in the context of your application
TEST_F(QuaternionOperatorTest, ScalarDivisionCommutes) {
    Quaternion result1 = 0.5f / q1;
    Quaternion result2 = q1 / 0.5f;
    ASSERT_TRUE(result1 == result2);
}
