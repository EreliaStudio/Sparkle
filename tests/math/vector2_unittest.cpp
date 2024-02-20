#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "math/spk_vector2.hpp"

using spk::Vector2;

namespace rc {

  template<>
  struct Arbitrary<Vector2> {
    static Gen<Vector2> arbitrary() {
      return gen::construct<Vector2>(
        gen::arbitrary<float>(),
        gen::arbitrary<float>()
      );
    }
  };
}

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

RC_GTEST_PROP(Vector2Test, AdditionIsCommutative, (const Vector2 &a, const Vector2 &b)) {
    RC_ASSERT(a + b == b + a);
}

RC_GTEST_PROP(Vector2Test, AdditionIsAssociative, (const Vector2 &a, const Vector2 &b, const Vector2 &c)) {
    RC_ASSERT((a + (b + c)) == ((a + b) + c));
}