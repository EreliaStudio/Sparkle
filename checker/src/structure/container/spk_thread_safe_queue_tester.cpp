#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "structure/container/spk_thread_safe_queue.hpp"

TEST(ThreadSafeQueueTest, PushPopSingleThread)
{
	spk::ThreadSafeQueue<int> queue;
	queue.push(42);
	EXPECT_EQ(queue.empty(), false) << "Queue should not report empty after push";

	int value = queue.pop();
	EXPECT_EQ(value, 42) << "Pop should return the pushed value";
	EXPECT_EQ(queue.empty(), true) << "Queue should be empty after popping all elements";
}

TEST(ThreadSafeQueueTest, MaintainsFifoOrder)
{
	spk::ThreadSafeQueue<int> queue;
	for (int i = 0; i < 5; ++i)
	{
		queue.push(i);
	}

	for (int i = 0; i < 5; ++i)
	{
		int value = queue.pop();
		EXPECT_EQ(value, i) << "Pop should return elements in FIFO order";
	}
}

TEST(ThreadSafeQueueTest, BlockingPopWaitsForData)
{
	spk::ThreadSafeQueue<int> queue;

	std::promise<void> started;
	auto future = std::async(
		std::launch::async,
		[&queue, &started]()
		{
			started.set_value();
			return queue.pop();
		});

	started.get_future().wait();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	queue.push(77);

	int value = future.get();
	EXPECT_EQ(value, 77) << "Pop should return the value once it becomes available";
}

TEST(ThreadSafeQueueTest, SupportsMoveOnlyTypes)
{
	spk::ThreadSafeQueue<std::unique_ptr<int>> queue;
	auto value = std::make_unique<int>(99);
	queue.push(std::move(value));
	EXPECT_EQ(value.get(), nullptr) << "Original unique_ptr should be cleared after move";

	auto result = queue.pop();
	ASSERT_NE(result, nullptr) << "Popped unique_ptr should not be null";
	EXPECT_EQ(*result, 99) << "Popped value should match the pushed pointer content";
}
