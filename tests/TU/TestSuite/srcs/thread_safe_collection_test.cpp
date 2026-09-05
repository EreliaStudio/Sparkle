#include <gtest/gtest.h>

#include "container/thread_safe_collection.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
	struct Record
	{
		int id;
		std::string name;

		Record(int p_id, std::string p_name) :
			id(p_id),
			name(std::move(p_name))
		{
		}
	};
}

TEST(ThreadSafeCollectionTest, StandardUsageEmplacesGetsErasesAndReusesKey)
{
	spk::ThreadSafeCollection<std::string, int> collection;

	auto inserted = collection.emplace("value", 12);
	ASSERT_NE(inserted, nullptr);
	EXPECT_EQ(*inserted, 12);
	EXPECT_TRUE(collection.contains("value"));
	EXPECT_EQ(collection.get("value"), inserted);
	EXPECT_EQ(collection.tryGet("value"), inserted);

	collection.erase("value");
	EXPECT_FALSE(collection.contains("value"));
	EXPECT_EQ(collection.tryGet("value"), nullptr);

	auto reused = collection.emplace("value", 42);
	ASSERT_NE(reused, nullptr);
	EXPECT_EQ(*reused, 42);
	EXPECT_EQ(collection.get("value"), reused);
}

TEST(ThreadSafeCollectionTest, EmplaceForwardsConstructionArguments)
{
	spk::ThreadSafeCollection<int, Record> collection;

	auto record = collection.emplace(7, 19, "record");
	ASSERT_NE(record, nullptr);
	EXPECT_EQ(record->id, 19);
	EXPECT_EQ(record->name, "record");
}

TEST(ThreadSafeCollectionTest, SupportsMoveOnlyValues)
{
	spk::ThreadSafeCollection<int, std::unique_ptr<int>> collection;

	auto value = collection.emplace(1, std::make_unique<int>(55));
	ASSERT_NE(value, nullptr);
	ASSERT_NE(*value, nullptr);
	EXPECT_EQ(**value, 55);
}

TEST(ThreadSafeCollectionTest, ErasingMissingKeyIsANoOp)
{
	spk::ThreadSafeCollection<int, int> collection;

	EXPECT_NO_THROW(collection.erase(99));
	EXPECT_FALSE(collection.contains(99));
}

TEST(ThreadSafeCollectionTest, SharedOwnershipRemainsStableAfterErase)
{
	spk::ThreadSafeCollection<int, Record> collection;
	auto retained = collection.emplace(1, 10, "kept");

	collection.erase(1);

	EXPECT_FALSE(collection.contains(1));
	ASSERT_NE(retained, nullptr);
	EXPECT_EQ(retained->id, 10);
	EXPECT_EQ(retained->name, "kept");
}

TEST(ThreadSafeCollectionTest, DuplicateKeyThrowsLogicError)
{
	spk::ThreadSafeCollection<int, int> collection;
	collection.emplace(1, 10);

	EXPECT_THROW(collection.emplace(1, 20), std::logic_error);
	EXPECT_EQ(*collection.get(1), 10);
}

TEST(ThreadSafeCollectionTest, MissingGetThrowsAndTryGetReturnsNull)
{
	spk::ThreadSafeCollection<int, int> collection;

	EXPECT_THROW((void)collection.get(404), std::out_of_range);
	EXPECT_EQ(collection.tryGet(404), nullptr);
}

TEST(ThreadSafeCollectionTest, ConcurrentReadersAndWritersCanUseDistinctAndSharedKeys)
{
	constexpr int writerCount = 4;
	constexpr int valuesPerWriter = 250;

	spk::ThreadSafeCollection<int, int> collection;
	collection.emplace(-1, 777);

	std::atomic<bool> failed = false;
	std::vector<std::jthread> readers;
	for (int readerIndex = 0; readerIndex < 4; ++readerIndex)
	{
		readers.emplace_back([&]() {
			for (int iteration = 0; iteration < 2000; ++iteration)
			{
				auto shared = collection.tryGet(-1);
				if (shared == nullptr || *shared != 777)
				{
					failed.store(true, std::memory_order_release);
				}
			}
		});
	}

	std::vector<std::jthread> writers;
	for (int writerIndex = 0; writerIndex < writerCount; ++writerIndex)
	{
		writers.emplace_back([&, writerIndex]() {
			const int base = writerIndex * valuesPerWriter;
			for (int offset = 0; offset < valuesPerWriter; ++offset)
			{
				const int key = base + offset;
				auto value = collection.emplace(key, key * 2);
				if (*value != key * 2)
				{
					failed.store(true, std::memory_order_release);
				}
			}
		});
	}

	for (auto &writer : writers)
	{
		writer.join();
	}
	for (auto &reader : readers)
	{
		reader.join();
	}

	EXPECT_FALSE(failed.load(std::memory_order_acquire));
	EXPECT_TRUE(collection.contains(-1));
	EXPECT_EQ(*collection.get(-1), 777);

	for (int key = 0; key < writerCount * valuesPerWriter; ++key)
	{
		ASSERT_TRUE(collection.contains(key));
		EXPECT_EQ(*collection.get(key), key * 2);
	}
}
