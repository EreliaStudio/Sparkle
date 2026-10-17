#include "container/double_buffer.hpp"

#include <gtest/gtest.h>

#include <latch>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <vector>

TEST(DoubleBufferTest, InitialReadBufferIsEmpty)
{
	spk::DoubleBuffer<std::unordered_set<int>> buffer;
	EXPECT_TRUE(buffer.read().empty());
	buffer.swap();
	EXPECT_TRUE(buffer.read().empty());
}

TEST(DoubleBufferTest, SwappingMakesPendingWritesReadable)
{
	spk::DoubleBuffer<std::unordered_set<int>> buffer;
	buffer.modify([](auto &pending) {
		pending.insert(5);
		pending.insert(7);
	});
	EXPECT_TRUE(buffer.read().empty());
	buffer.swap();
	EXPECT_TRUE(buffer.read().contains(5));
	EXPECT_TRUE(buffer.read().contains(7));
	EXPECT_EQ(buffer.read().size(), 2u);
}

TEST(DoubleBufferTest, WritesAfterSwapDoNotChangeCurrentReadBatch)
{
	spk::DoubleBuffer<std::unordered_set<int>> buffer;
	buffer.modify([](auto &pending) {
		pending.insert(1);
	});
	buffer.swap();
	buffer.modify([](auto &pending) {
		pending.insert(2);
	});
	EXPECT_TRUE(buffer.read().contains(1));
	EXPECT_FALSE(buffer.read().contains(2));
	buffer.read().clear();
	buffer.swap();
	EXPECT_TRUE(buffer.read().contains(2));
	EXPECT_FALSE(buffer.read().contains(1));
}

TEST(DoubleBufferTest, ModificationExceptionDoesNotLeaveBufferLocked)
{
	spk::DoubleBuffer<std::vector<int>> buffer;
	EXPECT_THROW(buffer.modify([](auto &pending) {
		pending.push_back(3);
		throw std::runtime_error("Injected modification failure.");
	}), std::runtime_error);
	EXPECT_NO_THROW(buffer.modify([](auto &pending) {
		pending.push_back(4);
	}));
	buffer.swap();
	ASSERT_EQ(buffer.read().size(), 2u);
	EXPECT_EQ(buffer.read()[0], 3);
	EXPECT_EQ(buffer.read()[1], 4);
}

TEST(DoubleBufferStressTest, ConcurrentWritersContributeAllDistinctValues)
{
	constexpr int Workers = 8;
	constexpr int ValuesPerWorker = 250;
	spk::DoubleBuffer<std::unordered_set<int>> buffer;
	std::latch start(1);
	std::vector<std::thread> workers;
	for (int worker = 0; worker < Workers; ++worker)
	{
		workers.emplace_back([&, worker]() {
			start.wait();
			for (int value = 0; value < ValuesPerWorker; ++value)
			{
				buffer.modify([&](auto &pending) {
					pending.insert(worker * ValuesPerWorker + value);
				});
			}
		});
	}
	start.count_down();
	for (auto &worker : workers)
	{
		worker.join();
	}
	buffer.swap();
	EXPECT_EQ(buffer.read().size(), static_cast<std::size_t>(Workers * ValuesPerWorker));
	for (int id = 0; id < Workers * ValuesPerWorker; ++id)
	{
		EXPECT_TRUE(buffer.read().contains(id));
	}
}
