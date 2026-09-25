#include <container/thread_safe_queue.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
	static_assert(
		std::is_copy_constructible_v<
			spk::ThreadSafeQueue<int>::Producer>);
	static_assert(
		!std::is_copy_constructible_v<
			spk::ThreadSafeQueue<int>::Consumer>);
	static_assert(
		std::is_move_constructible_v<
			spk::ThreadSafeQueue<int>::Consumer>);
}

TEST(ThreadSafeQueue, DirectInterfacePreservesFifoOrder)
{
	spk::ThreadSafeQueue<int> queue;

	queue.publish(3);
	queue.publish(7);
	queue.emplace(11);

	const auto first = queue.waitPop();
	const auto second = queue.waitPop();
	const auto third = queue.waitPop();

	ASSERT_TRUE(first.has_value());
	ASSERT_TRUE(second.has_value());
	ASSERT_TRUE(third.has_value());
	EXPECT_EQ(*first, 3);
	EXPECT_EQ(*second, 7);
	EXPECT_EQ(*third, 11);
}

TEST(ThreadSafeQueue, ProducerConsumerEndpointsPreserveFifoOrder)
{
	auto endpoints = spk::ThreadSafeQueue<int>::create();

	endpoints.producer.publish(2);
	endpoints.producer.emplace(4);
	endpoints.producer.publish(8);

	const auto first = endpoints.consumer.waitPop();
	const auto second = endpoints.consumer.waitPop();
	const auto third = endpoints.consumer.waitPop();

	ASSERT_TRUE(first.has_value());
	ASSERT_TRUE(second.has_value());
	ASSERT_TRUE(third.has_value());
	EXPECT_EQ(*first, 2);
	EXPECT_EQ(*second, 4);
	EXPECT_EQ(*third, 8);
}

TEST(ThreadSafeQueue, SupportsMoveOnlyValues)
{
	spk::ThreadSafeQueue<std::unique_ptr<int>> queue;

	queue.publish(std::make_unique<int>(19));
	queue.emplace(std::make_unique<int>(23));

	auto first = queue.waitPop();
	auto second = queue.waitPop();

	ASSERT_TRUE(first.has_value());
	ASSERT_TRUE(second.has_value());
	ASSERT_NE(*first, nullptr);
	ASSERT_NE(*second, nullptr);
	EXPECT_EQ(**first, 19);
	EXPECT_EQ(**second, 23);
}

TEST(ThreadSafeQueue, EndpointsKeepStateAliveWithoutOwningContainer)
{
	auto endpoints = [] {
		spk::ThreadSafeQueue<int> queue;
		return spk::ThreadSafeQueue<int>::Endpoints{
			.producer = queue.producer(),
			.consumer = queue.consumer()};
	}();

	endpoints.producer.publish(31);
	const auto value = endpoints.consumer.waitPop();

	ASSERT_TRUE(value.has_value());
	EXPECT_EQ(*value, 31);
}

TEST(ThreadSafeQueue, WaitPopReturnsImmediatelyForQueuedValue)
{
	spk::ThreadSafeQueue<int> queue;
	queue.publish(13);

	const auto value = queue.waitPop();

	ASSERT_TRUE(value.has_value());
	EXPECT_EQ(*value, 13);
}

TEST(ThreadSafeQueue, WaitPopWakesWhenProducerPublishes)
{
	auto endpoints = spk::ThreadSafeQueue<int>::create();
	std::promise<void> waiterStarted;
	std::promise<int> received;
	auto started = waiterStarted.get_future();
	auto result = received.get_future();

	std::jthread waiter(
		[consumer = std::move(endpoints.consumer),
		 &waiterStarted,
		 &received]() mutable {
			waiterStarted.set_value();
			const auto value = consumer.waitPop();
			received.set_value(value.value_or(-1));
		});

	started.wait();
	endpoints.producer.publish(73);
	EXPECT_EQ(result.get(), 73);
}

TEST(ThreadSafeQueue, StopTokenUnblocksEmptyConsumer)
{
	auto endpoints = spk::ThreadSafeQueue<int>::create();
	std::promise<bool> stopped;
	auto result = stopped.get_future();

	std::jthread waiter(
		[consumer = std::move(endpoints.consumer),
		 &stopped](std::stop_token stopToken) mutable {
			stopped.set_value(
				!consumer.waitPop(stopToken).has_value());
		});

	waiter.request_stop();
	EXPECT_TRUE(result.get());
}

TEST(ThreadSafeQueue, AlreadyQueuedValueWinsOverRequestedStop)
{
	spk::ThreadSafeQueue<int> queue;
	std::stop_source stopSource;

	queue.publish(91);
	stopSource.request_stop();

	const auto value = queue.waitPop(stopSource.get_token());

	ASSERT_TRUE(value.has_value());
	EXPECT_EQ(*value, 91);
}

TEST(ThreadSafeQueue, AlreadyRequestedStopReturnsEmptyWhenQueueIsEmpty)
{
	spk::ThreadSafeQueue<int> queue;
	std::stop_source stopSource;
	stopSource.request_stop();

	EXPECT_FALSE(
		queue.waitPop(stopSource.get_token()).has_value());
}

TEST(ThreadSafeQueue, MultipleProducersDeliverEveryValueExactlyOnce)
{
	auto endpoints = spk::ThreadSafeQueue<int>::create();
	constexpr int ProducerCount = 8;
	constexpr int ValuesPerProducer = 128;

	std::vector<std::thread> producers;
	for (int producerIndex = 0;
		 producerIndex < ProducerCount;
		 ++producerIndex)
	{
		producers.emplace_back(
			[producer = endpoints.producer,
			 producerIndex]() mutable {
				for (int valueIndex = 0; valueIndex < ValuesPerProducer; ++valueIndex)
				{
					producer.publish(
						producerIndex * ValuesPerProducer +
						valueIndex);
				}
			});
	}

	for (std::thread &producer : producers)
	{
		producer.join();
	}

	std::unordered_set<int> received;
	for (int index = 0;
		 index < ProducerCount * ValuesPerProducer;
		 ++index)
	{
		const auto value = endpoints.consumer.waitPop();
		ASSERT_TRUE(value.has_value());
		EXPECT_TRUE(received.insert(*value).second);
	}

	EXPECT_EQ(
		received.size(),
		static_cast<std::size_t>(
			ProducerCount * ValuesPerProducer));
}

TEST(ThreadSafeQueue, MultipleConsumersRemoveEachValueExactlyOnce)
{
	spk::ThreadSafeQueue<int> queue;
	constexpr int ValueCount = 1000;
	constexpr int ConsumerCount = 8;

	for (int value = 0; value < ValueCount; ++value)
	{
		queue.publish(value);
	}

	std::mutex valuesMutex;
	std::vector<int> values;
	values.reserve(ValueCount);
	std::atomic<int> consumed = 0;

	std::vector<std::jthread> consumers;
	for (int consumerIndex = 0;
		 consumerIndex < ConsumerCount;
		 ++consumerIndex)
	{
		consumers.emplace_back(
			[consumer = queue.consumer(),
			 &values,
			 &valuesMutex,
			 &consumed](std::stop_token stopToken) mutable {
				while (auto value =
						   consumer.waitPop(stopToken))
				{
					{
						const std::scoped_lock lock(
							valuesMutex);
						values.push_back(*value);
					}
					consumed.fetch_add(
						1,
						std::memory_order_release);
				}
			});
	}

	const auto deadline =
		std::chrono::steady_clock::now() +
		std::chrono::seconds(5);
	while (
		consumed.load(std::memory_order_acquire) <
			ValueCount &&
		std::chrono::steady_clock::now() < deadline)
	{
		std::this_thread::yield();
	}

	EXPECT_EQ(
		consumed.load(std::memory_order_acquire),
		ValueCount);

	for (std::jthread &consumer : consumers)
	{
		consumer.request_stop();
	}
	consumers.clear();

	std::ranges::sort(values);
	ASSERT_EQ(
		values.size(),
		static_cast<std::size_t>(ValueCount));
	for (int index = 0; index < ValueCount; ++index)
	{
		EXPECT_EQ(
			values[static_cast<std::size_t>(index)],
			index);
	}
}
