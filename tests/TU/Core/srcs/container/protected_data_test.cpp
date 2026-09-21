#include <gtest/gtest.h>

#include "container/protected_data.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <latch>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	using Data = spk::ProtectedData<int>;
}

static_assert(!std::is_copy_constructible_v<Data>);
static_assert(!std::is_move_constructible_v<Data>);
static_assert(!std::is_copy_assignable_v<Data>);
static_assert(!std::is_move_assignable_v<Data>);

static_assert(!std::is_copy_constructible_v<Data::Reader>);
static_assert(!std::is_move_constructible_v<Data::Reader>);
static_assert(!std::is_copy_assignable_v<Data::Reader>);
static_assert(!std::is_move_assignable_v<Data::Reader>);

static_assert(!std::is_copy_constructible_v<Data::Writer>);
static_assert(!std::is_move_constructible_v<Data::Writer>);
static_assert(!std::is_copy_assignable_v<Data::Writer>);
static_assert(!std::is_move_assignable_v<Data::Writer>);

TEST(ProtectedDataTest, StandardUsageReadsMutatesAndSupportsConcurrentReaders)
{
	Data data(12);

	{
		const auto reader = data.read();
		EXPECT_EQ(*reader, 12);
	}

	{
		auto writer = data.write();
		*writer = 42;
	}

	std::latch readersReady(2);
	std::latch releaseReaders(1);
	std::atomic<int> observedSum = 0;

	std::jthread first([&]() {
		const auto reader = data.read();
		observedSum.fetch_add(*reader, std::memory_order_relaxed);
		readersReady.count_down();
		releaseReaders.wait();
	});

	std::jthread second([&]() {
		const auto reader = data.read();
		observedSum.fetch_add(*reader, std::memory_order_relaxed);
		readersReady.count_down();
		releaseReaders.wait();
	});

	readersReady.wait();
	EXPECT_EQ(observedSum.load(std::memory_order_relaxed), 84);
	releaseReaders.count_down();
}

TEST(ProtectedDataTest, MultipleReadersCanHoldTheLockSimultaneously)
{
	Data data(7);
	auto firstReader = data.read();

	std::promise<void> acquired;
	auto acquiredFuture = acquired.get_future();

	std::jthread secondReader([&]() {
		auto reader = data.read();
		(void)reader;
		acquired.set_value();
	});

	EXPECT_EQ(acquiredFuture.wait_for(1s), std::future_status::ready);
	EXPECT_EQ(*firstReader, 7);
}

TEST(ProtectedDataTest, ReaderExcludesWriterUntilReaderLeavesScope)
{
	Data data(3);
	std::promise<void> attempted;
	std::promise<void> acquired;
	auto attemptedFuture = attempted.get_future();
	auto acquiredFuture = acquired.get_future();

	std::jthread writer;
	{
		auto reader = data.read();
		writer = std::jthread([&]() {
			attempted.set_value();
			auto guard = data.write();
			*guard = 9;
			acquired.set_value();
		});

		ASSERT_EQ(attemptedFuture.wait_for(1s), std::future_status::ready);
		EXPECT_EQ(acquiredFuture.wait_for(25ms), std::future_status::timeout);
		EXPECT_EQ(*reader, 3);
	}

	EXPECT_EQ(acquiredFuture.wait_for(1s), std::future_status::ready);
	writer.join();

	auto reader = data.read();
	EXPECT_EQ(*reader, 9);
}

TEST(ProtectedDataTest, WriterExcludesReadersAndOtherWritersUntilScopeExit)
{
	Data data(5);

	std::promise<void> readerAttempted;
	std::promise<void> readerAcquired;
	std::promise<void> writerAttempted;
	std::promise<void> writerAcquired;

	auto readerAttemptedFuture = readerAttempted.get_future();
	auto readerAcquiredFuture = readerAcquired.get_future();
	auto writerAttemptedFuture = writerAttempted.get_future();
	auto writerAcquiredFuture = writerAcquired.get_future();

	std::jthread readerThread;
	std::jthread writerThread;
	{
		auto writer = data.write();

		readerThread = std::jthread([&]() {
			readerAttempted.set_value();
			auto guard = data.read();
			(void)guard;
			readerAcquired.set_value();
		});

		writerThread = std::jthread([&]() {
			writerAttempted.set_value();
			auto guard = data.write();
			++(*guard);
			writerAcquired.set_value();
		});

		ASSERT_EQ(readerAttemptedFuture.wait_for(1s), std::future_status::ready);
		ASSERT_EQ(writerAttemptedFuture.wait_for(1s), std::future_status::ready);
		EXPECT_EQ(readerAcquiredFuture.wait_for(25ms), std::future_status::timeout);
		EXPECT_EQ(writerAcquiredFuture.wait_for(25ms), std::future_status::timeout);
		EXPECT_EQ(*writer, 5);
	}

	EXPECT_EQ(readerAcquiredFuture.wait_for(1s), std::future_status::ready);
	EXPECT_EQ(writerAcquiredFuture.wait_for(1s), std::future_status::ready);
	readerThread.join();
	writerThread.join();
}

TEST(ProtectedDataTest, LocksAreReleasedAutomaticallyAtScopeExit)
{
	Data data(1);

	{
		auto writer = data.write();
		*writer = 2;
	}

	{
		auto reader = data.read();
		EXPECT_EQ(*reader, 2);
	}

	{
		auto writer = data.write();
		*writer = 3;
	}

	EXPECT_EQ(*data.read(), 3);
}

TEST(ProtectedDataTest, ConcurrentReadersAndWritersProduceDeterministicFinalState)
{
	constexpr int writerCount = 4;
	constexpr int incrementsPerWriter = 1000;

	Data data(0);
	std::atomic<bool> running = true;
	std::atomic<bool> readerObservedInvalidValue = false;

	std::vector<std::jthread> readers;
	for (int index = 0; index < 3; ++index)
	{
		readers.emplace_back([&]() {
			while (running.load(std::memory_order_acquire))
			{
				const auto guard = data.read();
				if (*guard < 0 || *guard > writerCount * incrementsPerWriter)
				{
					readerObservedInvalidValue.store(true, std::memory_order_release);
				}
			}
		});
	}

	std::vector<std::jthread> writers;
	for (int index = 0; index < writerCount; ++index)
	{
		writers.emplace_back([&]() {
			for (int iteration = 0; iteration < incrementsPerWriter; ++iteration)
			{
				auto guard = data.write();
				++(*guard);
			}
		});
	}

	for (auto &writer : writers)
	{
		writer.join();
	}

	running.store(false, std::memory_order_release);
	for (auto &reader : readers)
	{
		reader.join();
	}

	EXPECT_FALSE(readerObservedInvalidValue.load(std::memory_order_acquire));
	EXPECT_EQ(*data.read(), writerCount * incrementsPerWriter);
}

TEST(ProtectedDataTest, OwnerMustOutliveReaderAndWriterGuards)
{
	// The public contract explicitly documents a Reader/Writer outliving its
	// originating ProtectedData as undefined behavior. The suite deliberately
	// does not execute that misuse.
	SUCCEED();
}
