#include <gtest/gtest.h>

#include "container/histogram.hpp"

namespace
{
	using Histogram = spk::Histogram<int>;

	TEST(HistogramTest, ConstructorStoresStep)
	{
		Histogram histogram(10);

		EXPECT_EQ(histogram.step(), 10);
		EXPECT_EQ(histogram.nbSample(), 0);
		EXPECT_TRUE(histogram.buckets().empty());
	}

	TEST(HistogramTest, ZeroStepThrows)
	{
		EXPECT_THROW(
			Histogram(0),
			spk::Exception);
	}

	TEST(HistogramTest, NegativeStepThrows)
	{
		EXPECT_THROW(
			Histogram(-10),
			spk::Exception);
	}

	TEST(HistogramTest, InsertCreatesExpectedBucket)
	{
		Histogram histogram(10);

		histogram.insert(15);

		ASSERT_EQ(histogram.buckets().size(), 1);
		ASSERT_TRUE(histogram.buckets().contains(1));

		const auto &bucket = histogram.buckets().at(1);

		EXPECT_EQ(bucket.lowerBound, 10);
		EXPECT_EQ(bucket.upperBound, 20);
		EXPECT_EQ(bucket.count, 1);
	}

	TEST(HistogramTest, LowerBoundIsInclusive)
	{
		Histogram histogram(10);

		histogram.insert(10);

		ASSERT_TRUE(histogram.buckets().contains(1));

		EXPECT_EQ(histogram.buckets().at(1).count, 1);
	}

	TEST(HistogramTest, UpperBoundIsExclusive)
	{
		Histogram histogram(10);

		histogram.insert(9);
		histogram.insert(10);

		ASSERT_TRUE(histogram.buckets().contains(0));
		ASSERT_TRUE(histogram.buckets().contains(1));

		EXPECT_EQ(histogram.buckets().at(0).count, 1);
		EXPECT_EQ(histogram.buckets().at(1).count, 1);
	}

	TEST(HistogramTest, MultipleValuesIncrementSameBucket)
	{
		Histogram histogram(10);

		histogram.insert(11);
		histogram.insert(15);
		histogram.insert(19);

		ASSERT_TRUE(histogram.buckets().contains(1));

		EXPECT_EQ(histogram.buckets().at(1).count, 3);
		EXPECT_EQ(histogram.nbSample(), 3);
	}

	TEST(HistogramTest, MultipleBucketsAreCreated)
	{
		Histogram histogram(10);

		histogram.insert(5);
		histogram.insert(15);
		histogram.insert(25);
		histogram.insert(35);

		ASSERT_EQ(histogram.buckets().size(), 4);

		EXPECT_EQ(histogram.buckets().at(0).count, 1);
		EXPECT_EQ(histogram.buckets().at(1).count, 1);
		EXPECT_EQ(histogram.buckets().at(2).count, 1);
		EXPECT_EQ(histogram.buckets().at(3).count, 1);
	}

	TEST(HistogramTest, EmptyBucketsAreNotCreated)
	{
		Histogram histogram(10);

		histogram.insert(5);
		histogram.insert(105);

		ASSERT_EQ(histogram.buckets().size(), 2);

		EXPECT_TRUE(histogram.buckets().contains(0));
		EXPECT_TRUE(histogram.buckets().contains(10));

		EXPECT_FALSE(histogram.buckets().contains(1));
		EXPECT_FALSE(histogram.buckets().contains(5));
		EXPECT_FALSE(histogram.buckets().contains(9));
	}

	TEST(HistogramTest, LargeValueDoesNotCreateIntermediateBuckets)
	{
		Histogram histogram(10);

		histogram.insert(1'000'000);

		ASSERT_EQ(histogram.buckets().size(), 1);
		ASSERT_TRUE(histogram.buckets().contains(100'000));

		const auto &bucket = histogram.buckets().at(100'000);

		EXPECT_EQ(bucket.lowerBound, 1'000'000);
		EXPECT_EQ(bucket.upperBound, 1'000'010);
		EXPECT_EQ(bucket.count, 1);
	}

	TEST(HistogramTest, NegativeValueThrows)
	{
		Histogram histogram(10);

		EXPECT_THROW(
			histogram.insert(-1),
			spk::Exception);

		EXPECT_EQ(histogram.nbSample(), 0);
		EXPECT_TRUE(histogram.buckets().empty());
	}

	TEST(HistogramTest, NbSampleTracksEveryInsertion)
	{
		Histogram histogram(10);

		histogram.insert(1);
		histogram.insert(2);
		histogram.insert(15);
		histogram.insert(42);

		EXPECT_EQ(histogram.nbSample(), 4);
	}
}