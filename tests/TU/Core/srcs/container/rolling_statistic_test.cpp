#include <gtest/gtest.h>

#include "container/rolling_statistic.hpp"

namespace
{
	using RollingStatistic = spk::RollingStatistic<int>;

	TEST(RollingStatisticTest, ConstructorCreatesEmptyWindow)
	{
		RollingStatistic window(5);

		EXPECT_EQ(window.capacity(), 5);
		EXPECT_EQ(window.nbSample(), 0);
		EXPECT_FALSE(window.full());
	}

	TEST(RollingStatisticTest, ZeroCapacityThrows)
	{
		EXPECT_THROW(
			RollingStatistic(0),
			spk::Exception);
	}

	TEST(RollingStatisticTest, EmptyWindowAverageThrows)
	{
		RollingStatistic window(5);

		EXPECT_THROW(
			auto notReceived = window.average(),
			spk::Exception);
	}

	TEST(RollingStatisticTest, EmptyWindowMinThrows)
	{
		RollingStatistic window(5);

		EXPECT_THROW(
			auto notReceived = window.min(),
			spk::Exception);
	}

	TEST(RollingStatisticTest, EmptyWindowMaxThrows)
	{
		RollingStatistic window(5);

		EXPECT_THROW(
			auto notReceived = window.max(),
			spk::Exception);
	}

	TEST(RollingStatisticTest, InsertSingleValue)
	{
		RollingStatistic window(5);

		window.insert(10);

		EXPECT_EQ(window.nbSample(), 1);
		EXPECT_EQ(window.average(), 10);
		EXPECT_EQ(window.min(), 10);
		EXPECT_EQ(window.max(), 10);
		EXPECT_FALSE(window.full());
	}

	TEST(RollingStatisticTest, InsertMultipleValues)
	{
		RollingStatistic window(5);

		window.insert(10);
		window.insert(20);
		window.insert(30);

		EXPECT_EQ(window.nbSample(), 3);
		EXPECT_EQ(window.average(), 20);
		EXPECT_EQ(window.min(), 10);
		EXPECT_EQ(window.max(), 30);
	}

	TEST(RollingStatisticTest, WindowBecomesFullAtCapacity)
	{
		RollingStatistic window(3);

		window.insert(10);
		window.insert(20);

		EXPECT_FALSE(window.full());

		window.insert(30);

		EXPECT_TRUE(window.full());
		EXPECT_EQ(window.nbSample(), 3);
	}

	TEST(RollingStatisticTest, InsertingPastCapacityRemovesOldestValue)
	{
		RollingStatistic window(3);

		window.insert(10);
		window.insert(20);
		window.insert(30);
		window.insert(40);

		EXPECT_EQ(window.nbSample(), 3);
		EXPECT_EQ(window.average(), 30);
		EXPECT_EQ(window.min(), 20);
		EXPECT_EQ(window.max(), 40);
	}

	TEST(RollingStatisticTest, MultipleWrapsKeepLatestValues)
	{
		RollingStatistic window(3);

		window.insert(10);
		window.insert(20);
		window.insert(30);
		window.insert(40);
		window.insert(50);
		window.insert(60);

		EXPECT_EQ(window.nbSample(), 3);
		EXPECT_EQ(window.average(), 50);
		EXPECT_EQ(window.min(), 40);
		EXPECT_EQ(window.max(), 60);
		EXPECT_TRUE(window.full());
	}

	TEST(RollingStatisticTest, ReplacingMinimumUpdatesMinimum)
	{
		RollingStatistic window(3);

		window.insert(10);
		window.insert(20);
		window.insert(30);

		EXPECT_EQ(window.min(), 10);

		window.insert(40);

		EXPECT_EQ(window.min(), 20);
	}

	TEST(RollingStatisticTest, ReplacingMaximumUpdatesMaximum)
	{
		RollingStatistic window(3);

		window.insert(30);
		window.insert(20);
		window.insert(10);

		EXPECT_EQ(window.max(), 30);

		window.insert(5);

		EXPECT_EQ(window.max(), 20);
	}

	TEST(RollingStatisticTest, CachedMinIsInvalidatedByInsertion)
	{
		RollingStatistic window(3);

		window.insert(20);
		window.insert(30);

		EXPECT_EQ(window.min(), 20);
		EXPECT_EQ(window.min(), 20);

		window.insert(10);

		EXPECT_EQ(window.min(), 10);
	}

	TEST(RollingStatisticTest, CachedMaxIsInvalidatedByInsertion)
	{
		RollingStatistic window(3);

		window.insert(10);
		window.insert(20);

		EXPECT_EQ(window.max(), 20);
		EXPECT_EQ(window.max(), 20);

		window.insert(30);

		EXPECT_EQ(window.max(), 30);
	}

	TEST(RollingStatisticTest, ResetClearsWindow)
	{
		RollingStatistic window(3);

		window.insert(10);
		window.insert(20);
		window.insert(30);

		window.reset();

		EXPECT_EQ(window.nbSample(), 0);
		EXPECT_EQ(window.capacity(), 3);
		EXPECT_FALSE(window.full());
	}

	TEST(RollingStatisticTest, ResetMakesValuesUnavailable)
	{
		RollingStatistic window(3);

		window.insert(10);
		window.insert(20);

		EXPECT_EQ(window.min(), 10);
		EXPECT_EQ(window.max(), 20);

		window.reset();

		EXPECT_THROW(
			auto notReceived = window.average(),
			spk::Exception);

		EXPECT_THROW(
			auto notReceived = window.min(),
			spk::Exception);

		EXPECT_THROW(
			auto notReceived = window.max(),
			spk::Exception);
	}

	TEST(RollingStatisticTest, InsertAfterResetStartsNewWindow)
	{
		RollingStatistic window(3);

		window.insert(10);
		window.insert(20);
		window.insert(30);

		window.reset();

		window.insert(100);
		window.insert(200);

		EXPECT_EQ(window.nbSample(), 2);
		EXPECT_EQ(window.average(), 150);
		EXPECT_EQ(window.min(), 100);
		EXPECT_EQ(window.max(), 200);
		EXPECT_FALSE(window.full());
	}

	TEST(RollingStatisticTest, ResetAfterWrapStartsNewWindow)
	{
		RollingStatistic window(3);

		window.insert(10);
		window.insert(20);
		window.insert(30);
		window.insert(40);
		window.insert(50);

		window.reset();

		window.insert(5);
		window.insert(15);
		window.insert(25);

		EXPECT_EQ(window.nbSample(), 3);
		EXPECT_EQ(window.average(), 15);
		EXPECT_EQ(window.min(), 5);
		EXPECT_EQ(window.max(), 25);
		EXPECT_TRUE(window.full());
	}

	TEST(RollingStatisticTest, CapacityOneAlwaysContainsLatestValue)
	{
		RollingStatistic window(1);

		window.insert(10);

		EXPECT_EQ(window.average(), 10);
		EXPECT_EQ(window.min(), 10);
		EXPECT_EQ(window.max(), 10);
		EXPECT_TRUE(window.full());

		window.insert(20);

		EXPECT_EQ(window.nbSample(), 1);
		EXPECT_EQ(window.average(), 20);
		EXPECT_EQ(window.min(), 20);
		EXPECT_EQ(window.max(), 20);
		EXPECT_TRUE(window.full());
	}

	TEST(RollingStatisticTest, HandlesNegativeValues)
	{
		RollingStatistic window(3);

		window.insert(-30);
		window.insert(-10);
		window.insert(-20);

		EXPECT_EQ(window.average(), -20);
		EXPECT_EQ(window.min(), -30);
		EXPECT_EQ(window.max(), -10);
	}

	TEST(RollingStatisticTest, MinAndMaxRemainCorrectAfterSeveralEvictions)
	{
		RollingStatistic window(4);

		window.insert(100);
		window.insert(20);
		window.insert(80);
		window.insert(40);

		EXPECT_EQ(window.min(), 20);
		EXPECT_EQ(window.max(), 100);

		window.insert(60);

		EXPECT_EQ(window.min(), 20);
		EXPECT_EQ(window.max(), 80);

		window.insert(70);

		EXPECT_EQ(window.min(), 40);
		EXPECT_EQ(window.max(), 80);

		window.insert(50);

		EXPECT_EQ(window.min(), 40);
		EXPECT_EQ(window.max(), 70);
	}
}