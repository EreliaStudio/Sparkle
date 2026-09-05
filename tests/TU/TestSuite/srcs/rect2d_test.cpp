#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <sstream>

#include "math/rect2d.hpp"

namespace
{

	spk::Rect2D makeRect(std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height)
	{
		spk::Rect2D result;
		result.anchor = {x, y};
		result.size = {width, height};
		return result;
	}
	TEST(Rect2DTest, StandardUsageCreatesContainsMovesShrinksIntersectsComparesAndStreams)
	{
		const spk::Rect2D rect = makeRect(10, 20, 100u, 50u);

		EXPECT_TRUE(rect.contains({10, 20}));
		EXPECT_TRUE(rect.contains({109, 69}));
		EXPECT_FALSE(rect.contains({110, 20}));

		EXPECT_EQ(rect.atOrigin(), makeRect(0, 0, 100u, 50u));
		EXPECT_EQ(rect.shrink({5, 10}), makeRect(15, 30, 90u, 30u));
		EXPECT_EQ(rect.intersect(makeRect(50, 0, 100u, 50u)), makeRect(50, 20, 60u, 30u));
		EXPECT_EQ(rect, makeRect(10, 20, 100u, 50u));

		std::ostringstream stream;
		stream << rect;
		EXPECT_EQ(stream.str(), "(10, 20) - (100, 50)");
	}

	TEST(Rect2DTest, ContainsUsesInclusiveLeftTopAndExclusiveRightBottomBoundaries)
	{
		const spk::Rect2D rect = makeRect(-3, 4, 5u, 7u);

		EXPECT_TRUE(rect.contains({-3, 4}));
		EXPECT_TRUE(rect.contains({1, 10}));
		EXPECT_FALSE(rect.contains({-4, 4}));
		EXPECT_FALSE(rect.contains({-3, 3}));
		EXPECT_FALSE(rect.contains({2, 4}));
		EXPECT_FALSE(rect.contains({-3, 11}));
	}

	TEST(Rect2DTest, EmptyRectangleContainsNoPoint)
	{
		const spk::Rect2D emptyWidth = makeRect(2, 3, 0u, 4u);
		const spk::Rect2D emptyHeight = makeRect(2, 3, 4u, 0u);
		const spk::Rect2D emptyBoth = makeRect(2, 3, 0u, 0u);

		EXPECT_FALSE(emptyWidth.contains({2, 3}));
		EXPECT_FALSE(emptyHeight.contains({2, 3}));
		EXPECT_FALSE(emptyBoth.contains({2, 3}));
	}

	TEST(Rect2DTest, IntersectionCoversDisjointTouchingContainedAndIdenticalRectangles)
	{
		const spk::Rect2D base = makeRect(0, 0, 10u, 10u);

		EXPECT_EQ(base.intersect(makeRect(20, 20, 5u, 5u)), makeRect(20, 20, 0u, 0u));
		EXPECT_EQ(base.intersect(makeRect(10, 2, 5u, 5u)), makeRect(10, 2, 0u, 0u));
		EXPECT_EQ(base.intersect(makeRect(2, 3, 4u, 5u)), makeRect(2, 3, 4u, 5u));
		EXPECT_EQ(base.intersect(base), base);
	}

	TEST(Rect2DTest, IntersectionHandlesNegativeAnchors)
	{
		const spk::Rect2D first = makeRect(-10, -10, 15u, 15u);
		const spk::Rect2D second = makeRect(-5, -20, 20u, 20u);

		EXPECT_EQ(first.intersect(second), makeRect(-5, -10, 10u, 10u));
	}

	TEST(Rect2DTest, ShrinkMovesAnchorInwardAndReducesBothSides)
	{
		const spk::Rect2D rect = makeRect(10, 20, 20u, 30u);

		EXPECT_EQ(rect.shrink({2, 3}), makeRect(12, 23, 16u, 24u));
		EXPECT_EQ(rect.shrink({0, 0}), rect);
	}

	TEST(Rect2DTest, OversizedShrinkOffsetsClampResultSizeToZero)
	{
		const spk::Rect2D rect = makeRect(10, 20, 8u, 6u);

		EXPECT_EQ(rect.shrink({5, 1}), makeRect(15, 21, 0u, 0u));
		EXPECT_EQ(rect.shrink({1, 4}), makeRect(11, 24, 0u, 0u));
	}

	TEST(Rect2DTest, UnsignedSizeBoundaryRemainsCorrectWithinSignedArithmeticRange)
	{
		constexpr std::uint32_t largeWidth = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
		const spk::Rect2D rect = makeRect(-1, 0, largeWidth, 1u);

		EXPECT_TRUE(rect.contains({-1, 0}));
		EXPECT_TRUE(rect.contains({std::numeric_limits<std::int32_t>::max() - 2, 0}));
		EXPECT_FALSE(rect.contains({std::numeric_limits<std::int32_t>::max() - 1, 0}));
	}
}
