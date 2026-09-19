#include <gtest/gtest.h>

#include "container/thread_safe_slot.hpp"

#include <atomic>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

namespace
{
	struct Snapshot
	{
		int sequence = 0;
		int checksum = 0;
	};
}

TEST(ThreadSafeSlotTest, StandardUsagePublishesAndAcquiresLatestFromDirectAndSplitEndpoints)
{
	spk::ThreadSafeSlot<int> slot;
	slot.publish(12);

	auto direct = slot.acquireLatest();
	ASSERT_NE(direct, nullptr);
	EXPECT_EQ(*direct, 12);

	auto producer = slot.producer();
	auto consumer = slot.consumer();
	producer.publish(42);

	auto split = consumer.acquireLatest();
	ASSERT_NE(split, nullptr);
	EXPECT_EQ(*split, 42);
}

TEST(ThreadSafeSlotTest, UntouchedSlotReturnsNull)
{
	const spk::ThreadSafeSlot<int> slot;
	EXPECT_EQ(slot.acquireLatest(), nullptr);

	auto endpoints = spk::ThreadSafeSlot<int>::create();
	EXPECT_EQ(endpoints.consumer.acquireLatest(), nullptr);
}

TEST(ThreadSafeSlotTest, NewPublicationReplacesOlderSnapshot)
{
	spk::ThreadSafeSlot<int> slot;

	slot.publish(1);
	auto first = slot.acquireLatest();
	ASSERT_NE(first, nullptr);
	EXPECT_EQ(*first, 1);

	slot.publish(2);
	auto second = slot.acquireLatest();
	ASSERT_NE(second, nullptr);
	EXPECT_EQ(*second, 2);
	EXPECT_NE(first, second);
}

TEST(ThreadSafeSlotTest, AcquiredSnapshotRemainsAliveAndImmutableAfterReplacement)
{
	spk::ThreadSafeSlot<std::string> slot;

	slot.publish("old");
	auto oldSnapshot = slot.acquireLatest();
	ASSERT_NE(oldSnapshot, nullptr);

	slot.publish("new");
	auto newSnapshot = slot.acquireLatest();

	ASSERT_NE(newSnapshot, nullptr);
	EXPECT_EQ(*oldSnapshot, "old");
	EXPECT_EQ(*newSnapshot, "new");
}

TEST(ThreadSafeSlotTest, SupportsMoveOnlyValues)
{
	spk::ThreadSafeSlot<std::unique_ptr<int>> slot;

	slot.publish(std::make_unique<int>(77));

	auto snapshot = slot.acquireLatest();
	ASSERT_NE(snapshot, nullptr);
	ASSERT_NE(*snapshot, nullptr);
	EXPECT_EQ(**snapshot, 77);
}

TEST(ThreadSafeSlotTest, ConcurrentPublicationAndAcquisitionOnlyExposeCompleteSnapshots)
{
	constexpr int finalSequence = 10000;

	spk::ThreadSafeSlot<Snapshot> slot;
	std::atomic<bool> finished = false;
	std::atomic<bool> observedInvalidSnapshot = false;
	std::atomic<int> maximumObserved = 0;

	std::vector<std::jthread> readers;
	for (int index = 0; index < 4; ++index)
	{
		readers.emplace_back([&]() {
			while (!finished.load(std::memory_order_acquire))
			{
				auto snapshot = slot.acquireLatest();
				if (snapshot == nullptr)
				{
					continue;
				}

				if (snapshot->checksum != ~snapshot->sequence)
				{
					observedInvalidSnapshot.store(true, std::memory_order_release);
				}

				int current = maximumObserved.load(std::memory_order_relaxed);
				while (snapshot->sequence > current &&
					   !maximumObserved.compare_exchange_weak(
						   current,
						   snapshot->sequence,
						   std::memory_order_relaxed))
				{
				}
			}
		});
	}

	std::jthread writer([&]() {
		for (int sequence = 1; sequence <= finalSequence; ++sequence)
		{
			slot.publish(Snapshot{sequence, ~sequence});
		}
		finished.store(true, std::memory_order_release);
	});

	writer.join();
	for (auto &reader : readers)
	{
		reader.join();
	}

	auto finalSnapshot = slot.acquireLatest();
	ASSERT_NE(finalSnapshot, nullptr);
	EXPECT_EQ(finalSnapshot->sequence, finalSequence);
	EXPECT_EQ(finalSnapshot->checksum, ~finalSequence);
	EXPECT_FALSE(observedInvalidSnapshot.load(std::memory_order_acquire));
}

TEST(ThreadSafeSlotTest, CopiedEndpointsShareStateAndOutliveFactoryWrapper)
{
	auto endpoints = []() {
		spk::ThreadSafeSlot<int> slot;
		return spk::ThreadSafeSlot<int>::Endpoints{
			.producer = slot.producer(),
			.consumer = slot.consumer()};
	}();

	auto copiedProducer = endpoints.producer;
	auto copiedConsumer = endpoints.consumer;

	copiedProducer.publish(123);

	auto fromOriginalConsumer = endpoints.consumer.acquireLatest();
	auto fromCopiedConsumer = copiedConsumer.acquireLatest();
	ASSERT_NE(fromOriginalConsumer, nullptr);
	ASSERT_NE(fromCopiedConsumer, nullptr);
	EXPECT_EQ(*fromOriginalConsumer, 123);
	EXPECT_EQ(*fromCopiedConsumer, 123);

	endpoints.producer.publish(456);
	auto latest = copiedConsumer.acquireLatest();
	ASSERT_NE(latest, nullptr);
	EXPECT_EQ(*latest, 456);
}
