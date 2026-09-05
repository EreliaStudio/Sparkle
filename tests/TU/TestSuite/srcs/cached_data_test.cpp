#include <gtest/gtest.h>

#include <concepts>
#include "container/cached_data.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
	struct Record
	{
		int value = 0;
		std::string label;

		Record() = default;
		Record(int p_value, std::string p_label) : value(p_value), label(std::move(p_label)) {}
	};
}

TEST(CachedDataTest, StandardUsageGeneratesOnceInvalidatesAndRegeneratesLazily)
{
	int generationCount = 0;
	spk::CachedData<int> cache([&generationCount]() {
		return ++generationCount;
	});

	EXPECT_EQ(cache.get(), 1);
	EXPECT_EQ(cache.get(), 1);
	EXPECT_EQ(generationCount, 1);

	cache.invalidate();
	EXPECT_EQ(generationCount, 1);
	EXPECT_EQ(cache.get(), 2);
	EXPECT_EQ(generationCount, 2);
}

TEST(CachedDataTest, AccessorsConversionDereferenceAndArrowShareTheSameCachedValue)
{
	int generationCount = 0;
	spk::CachedData<Record> cache([&generationCount]() {
		++generationCount;
		return Record{42, "answer"};
	});

	EXPECT_EQ(cache.get().value, 42);
	EXPECT_EQ((*cache).label, "answer");
	EXPECT_EQ(cache->value, 42);
	const Record &converted = cache;
	EXPECT_EQ(converted.label, "answer");
	EXPECT_EQ(generationCount, 1);

	const auto &constCache = cache;
	EXPECT_EQ(constCache.get().value, 42);
	EXPECT_EQ((*constCache).label, "answer");
	EXPECT_EQ(constCache->value, 42);
}

TEST(CachedDataTest, RefreshDestroysOldValueAndImmediatelyRegenerates)
{
	int generationCount = 0;
	int destructionCount = 0;
	spk::CachedData<int> cache(
		[&generationCount]() { return ++generationCount; },
		[&destructionCount](int &) { ++destructionCount; });

	EXPECT_EQ(cache.get(), 1);
	EXPECT_EQ(cache.refresh(), 2);
	EXPECT_EQ(generationCount, 2);
	EXPECT_EQ(destructionCount, 1);
}

TEST(CachedDataTest, SetAndEmplaceDiscardPreviousCachedValuesExactlyOnce)
{
	int destructionCount = 0;
	{
		spk::CachedData<Record> cache(
			[]() { return Record{1, "generated"}; },
			[&destructionCount](Record &) { ++destructionCount; });

		(void)cache.get();
		cache.set(Record{2, "set"});
		EXPECT_EQ(destructionCount, 1);
		EXPECT_EQ(cache->value, 2);

		Record &record = cache.emplace(3, "emplaced");
		EXPECT_EQ(destructionCount, 2);
		EXPECT_EQ(record.value, 3);
		EXPECT_EQ(record.label, "emplaced");
	}
	EXPECT_EQ(destructionCount, 3);
}

TEST(CachedDataTest, TakeTransfersCurrentValueWithoutInvokingConfiguredDestructor)
{
	int destructionCount = 0;
	spk::CachedData<std::string> cache(
		[]() { return std::string("generated"); },
		[&destructionCount](std::string &) { ++destructionCount; });

	EXPECT_EQ(cache.get(), "generated");
	std::optional<std::string> value = cache.take();
	ASSERT_TRUE(value.has_value());
	EXPECT_EQ(*value, "generated");
	EXPECT_EQ(destructionCount, 0);

	EXPECT_EQ(cache.get(), "generated");
	EXPECT_EQ(destructionCount, 0);
	cache.invalidate();
	EXPECT_EQ(destructionCount, 1);
}

TEST(CachedDataTest, InvalidateAndTakeOnEmptyCacheAreNoOps)
{
	int destructionCount = 0;
	spk::CachedData<int> cache(
		[]() { return 12; },
		[&destructionCount](int &) { ++destructionCount; });

	cache.invalidate();
	EXPECT_EQ(destructionCount, 0);
	EXPECT_FALSE(cache.take().has_value());
	EXPECT_EQ(destructionCount, 0);
}

TEST(CachedDataTest, EmptyAndPopulatedCopyConstructionAreIndependent)
{
	int generationCount = 0;
	spk::CachedData<int> empty([&generationCount]() { return ++generationCount; });
	spk::CachedData<int> emptyCopy(empty);
	EXPECT_EQ(emptyCopy.get(), 1);
	EXPECT_EQ(generationCount, 1);

	spk::CachedData<int> populated([&generationCount]() { return ++generationCount; });
	EXPECT_EQ(populated.get(), 2);
	spk::CachedData<int> populatedCopy(populated);
	EXPECT_EQ(populatedCopy.get(), 2);
	populatedCopy.set(100);
	EXPECT_EQ(populated.get(), 2);
	EXPECT_EQ(populatedCopy.get(), 100);
}

TEST(CachedDataTest, CopyAssignmentDestroysDestinationAndSupportsSelfAssignment)
{
	int destructionCount = 0;
	spk::CachedData<int> source(
		[]() { return 11; },
		[&destructionCount](int &) { ++destructionCount; });
	spk::CachedData<int> destination(
		[]() { return 22; },
		[&destructionCount](int &) { ++destructionCount; });

	EXPECT_EQ(source.get(), 11);
	EXPECT_EQ(destination.get(), 22);
	destination = source;
	EXPECT_EQ(destructionCount, 1);
	EXPECT_EQ(destination.get(), 11);

	destination = destination;
	EXPECT_EQ(destination.get(), 11);
	EXPECT_EQ(destructionCount, 1);
}

TEST(CachedDataTest, MoveConstructionSupportsMoveOnlyValuesAndMovedFromReuseThroughSet)
{
	static_assert(std::move_constructible<spk::CachedData<std::unique_ptr<int>>>);
	static_assert(!std::copy_constructible<spk::CachedData<std::unique_ptr<int>>>);

	spk::CachedData<std::unique_ptr<int>> source([]() {
		return std::make_unique<int>(42);
	});
	EXPECT_EQ(**source, 42);

	spk::CachedData<std::unique_ptr<int>> destination(std::move(source));
	ASSERT_NE(destination.get(), nullptr);
	EXPECT_EQ(*destination.get(), 42);

	source.set(std::make_unique<int>(7));
	ASSERT_NE(source.get(), nullptr);
	EXPECT_EQ(*source.get(), 7);
}

TEST(CachedDataTest, MoveAssignmentDiscardsDestinationAndMovesCurrentValue)
{
	int destructionCount = 0;
	spk::CachedData<std::string> source(
		[]() { return std::string("source"); },
		[&destructionCount](std::string &) { ++destructionCount; });
	spk::CachedData<std::string> destination(
		[]() { return std::string("destination"); },
		[&destructionCount](std::string &) { ++destructionCount; });

	(void)source.get();
	(void)destination.get();
	destination = std::move(source);
	EXPECT_EQ(destructionCount, 1);
	EXPECT_EQ(destination.get(), "source");

	destination = std::move(destination);
	EXPECT_EQ(destination.get(), "source");
}


TEST(CachedDataTest, EmptyMoveConstructionAndAssignmentTransferGeneratorConfiguration)
{
	int generationCount = 0;
	spk::CachedData<int> source([&generationCount]() { return ++generationCount; });
	spk::CachedData<int> moved(std::move(source));
	EXPECT_EQ(moved.get(), 1);

	source.set(9);
	EXPECT_EQ(source.get(), 9);

	spk::CachedData<int> empty([&generationCount]() { return ++generationCount; });
	spk::CachedData<int> destination([]() { return 99; });
	EXPECT_EQ(destination.get(), 99);
	destination = std::move(empty);
	EXPECT_EQ(destination.get(), 2);
}

TEST(CachedDataTest, CopyAssignmentFromEmptyCacheCopiesGeneratorWithoutMaterializingSource)
{
	int generationCount = 0;
	spk::CachedData<int> source([&generationCount]() { return ++generationCount; });
	spk::CachedData<int> destination([]() { return 99; });
	EXPECT_EQ(destination.get(), 99);

	destination = source;
	EXPECT_EQ(generationCount, 0);
	EXPECT_EQ(destination.get(), 1);
	EXPECT_EQ(generationCount, 1);
}

TEST(CachedDataTest, EmptyCacheWithoutGeneratorThrowsDocumentedRuntimeError)
{
	spk::CachedData<int> cache;
	try
	{
		(void)cache.get();
		FAIL() << "Expected std::runtime_error";
	}
	catch (const std::runtime_error &exception)
	{
		EXPECT_STREQ(exception.what(), "CachedData: generator not set");
	}
}

TEST(CachedDataTest, BacklogRegenerateAndReleaseSemanticsAreProvidedByRefreshAndTake)
{
	int generationCount = 0;
	spk::CachedData<int> cache([&generationCount]() { return ++generationCount; });
	EXPECT_EQ(cache.get(), 1);
	EXPECT_EQ(cache.refresh(), 2);
	std::optional<int> released = cache.take();
	ASSERT_TRUE(released.has_value());
	EXPECT_EQ(*released, 2);
}
