#include <gtest/gtest.h>

#include "container/thread_safe_fifo.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <memory>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	struct Message
	{
		int producer = 0;
		int sequence = 0;
	};
}

using IntFIFO = spk::ThreadSafeFIFO<int>;

static_assert(!std::is_copy_constructible_v<IntFIFO::Consumer>);
static_assert(std::is_move_constructible_v<IntFIFO::Consumer>);
static_assert(!std::is_copy_assignable_v<IntFIFO::Consumer>);
static_assert(std::is_move_assignable_v<IntFIFO::Consumer>);

TEST(ThreadSafeFIFOTest, StandardUsagePublishesWaitsDrainsInOrderAndReusesChannel)
{
	auto endpoints = IntFIFO::create();

	endpoints.producer.publish(1);
	endpoints.producer.emplace(2);
	endpoints.producer.publish(3);

	ASSERT_TRUE(endpoints.consumer.wait());
	auto &firstDrain = endpoints.consumer.drain();
	EXPECT_EQ(firstDrain, (std::vector<int>{1, 2, 3}));

	endpoints.producer.emplace(4);
	ASSERT_TRUE(endpoints.consumer.wait());
	auto &secondDrain = endpoints.consumer.drain();
	EXPECT_EQ(secondDrain, (std::vector<int>{4}));
}

TEST(ThreadSafeFIFOTest, WrapperSupportsDirectPublicationWaitingAndDrain)
{
	IntFIFO fifo;
	std::vector<int> values;

	fifo.publish(8);
	fifo.emplace(9);

	EXPECT_TRUE(fifo.wait());
	EXPECT_EQ(fifo.drain(values), (std::vector<int>{8, 9}));
}

TEST(ThreadSafeFIFOTest, EmptyDrainReturnsEmptyReusableBuffer)
{
	auto endpoints = IntFIFO::create();

	auto &first = endpoints.consumer.drain();
	EXPECT_TRUE(first.empty());

	endpoints.producer.publish(5);
	auto &second = endpoints.consumer.drain();
	EXPECT_EQ(&first, &second);
	EXPECT_EQ(second, (std::vector<int>{5}));

	auto &third = endpoints.consumer.drain();
	EXPECT_EQ(&second, &third);
	EXPECT_TRUE(third.empty());
}

TEST(ThreadSafeFIFOTest, SupportsMoveOnlyValues)
{
	auto endpoints = spk::ThreadSafeFIFO<std::unique_ptr<int>>::create();

	endpoints.producer.publish(std::make_unique<int>(12));
	endpoints.producer.emplace(new int(42));

	auto &values = endpoints.consumer.drain();
	ASSERT_EQ(values.size(), 2u);
	ASSERT_NE(values[0], nullptr);
	ASSERT_NE(values[1], nullptr);
	EXPECT_EQ(*values[0], 12);
	EXPECT_EQ(*values[1], 42);
}

TEST(ThreadSafeFIFOTest, MultipleProducersPreserveEachProducersLocalOrder)
{
	constexpr int producerCount = 4;
	constexpr int messageCount = 250;

	auto endpoints = spk::ThreadSafeFIFO<Message>::create();
	std::vector<spk::ThreadSafeFIFO<Message>::Producer> producers(
		producerCount,
		endpoints.producer);

	std::vector<std::jthread> threads;
	for (int producerIndex = 0; producerIndex < producerCount; ++producerIndex)
	{
		threads.emplace_back([producer = producers[producerIndex], producerIndex]() mutable {
			for (int sequence = 0; sequence < messageCount; ++sequence)
			{
				producer.emplace(producerIndex, sequence);
			}
		});
	}

	for (auto &thread : threads)
	{
		thread.join();
	}

	auto &values = endpoints.consumer.drain();
	ASSERT_EQ(values.size(), static_cast<std::size_t>(producerCount * messageCount));

	std::vector<int> nextExpected(producerCount, 0);
	for (const Message &message : values)
	{
		ASSERT_GE(message.producer, 0);
		ASSERT_LT(message.producer, producerCount);
		EXPECT_EQ(message.sequence, nextExpected[message.producer]);
		++nextExpected[message.producer];
	}

	for (int producerIndex = 0; producerIndex < producerCount; ++producerIndex)
	{
		EXPECT_EQ(nextExpected[producerIndex], messageCount);
	}
}

TEST(ThreadSafeFIFOTest, ValuesPublishedBetweenDrainsAppearInNextBatch)
{
	auto endpoints = IntFIFO::create();

	endpoints.producer.publish(1);
	endpoints.producer.publish(2);
	EXPECT_EQ(endpoints.consumer.drain(), (std::vector<int>{1, 2}));

	endpoints.producer.publish(3);
	endpoints.producer.publish(4);
	EXPECT_EQ(endpoints.consumer.drain(), (std::vector<int>{3, 4}));
}

TEST(ThreadSafeFIFOTest, EndpointsRemainAliveAfterOriginatingWrapperIsDestroyed)
{
	auto endpoints = []() {
		IntFIFO fifo;
		return IntFIFO::Endpoints{
			.producer = fifo.producer(),
			.consumer = fifo.consumer()};
	}();

	endpoints.producer.publish(91);
	ASSERT_TRUE(endpoints.consumer.wait());
	EXPECT_EQ(endpoints.consumer.drain(), (std::vector<int>{91}));
}

TEST(ThreadSafeFIFOTest, WaitBlocksUntilPublication)
{
	auto endpoints = IntFIFO::create();

	std::promise<void> started;
	auto startedFuture = started.get_future();
	auto waitFuture = std::async(std::launch::async, [&]() {
		started.set_value();
		return endpoints.consumer.wait();
	});

	ASSERT_EQ(startedFuture.wait_for(1s), std::future_status::ready);
	EXPECT_EQ(waitFuture.wait_for(25ms), std::future_status::timeout);

	endpoints.producer.publish(15);

	ASSERT_EQ(waitFuture.wait_for(1s), std::future_status::ready);
	EXPECT_TRUE(waitFuture.get());
	EXPECT_EQ(endpoints.consumer.drain(), (std::vector<int>{15}));
}

TEST(ThreadSafeFIFOTest, WaitReturnsFalseWhenStopIsRequested)
{
	auto endpoints = IntFIFO::create();
	std::stop_source stopSource;

	std::promise<void> started;
	auto startedFuture = started.get_future();
	auto waitFuture = std::async(std::launch::async, [&]() {
		started.set_value();
		return endpoints.consumer.wait(stopSource.get_token());
	});

	ASSERT_EQ(startedFuture.wait_for(1s), std::future_status::ready);
	EXPECT_EQ(waitFuture.wait_for(25ms), std::future_status::timeout);

	stopSource.request_stop();

	ASSERT_EQ(waitFuture.wait_for(1s), std::future_status::ready);
	EXPECT_FALSE(waitFuture.get());
	EXPECT_TRUE(endpoints.consumer.drain().empty());
}

TEST(ThreadSafeFIFOTest, WaitHandlesAlreadyRequestedStop)
{
	auto endpoints = IntFIFO::create();
	std::stop_source stopSource;
	stopSource.request_stop();

	EXPECT_FALSE(endpoints.consumer.wait(stopSource.get_token()));
}

TEST(ThreadSafeFIFOTest, AlreadyPublishedValueWinsOverRequestedStop)
{
	auto endpoints = IntFIFO::create();
	std::stop_source stopSource;
	stopSource.request_stop();
	endpoints.producer.publish(6);

	EXPECT_TRUE(endpoints.consumer.wait(stopSource.get_token()));
	EXPECT_EQ(endpoints.consumer.drain(), (std::vector<int>{6}));
}

TEST(ThreadSafeFIFOTest, PublicationAroundWaitBoundaryDoesNotLoseNotification)
{
	for (int iteration = 0; iteration < 100; ++iteration)
	{
		auto endpoints = IntFIFO::create();

		std::promise<void> entered;
		auto enteredFuture = entered.get_future();
		auto waitFuture = std::async(std::launch::async, [&]() {
			entered.set_value();
			return endpoints.consumer.wait();
		});

		ASSERT_EQ(enteredFuture.wait_for(1s), std::future_status::ready);
		endpoints.producer.publish(iteration);

		ASSERT_EQ(waitFuture.wait_for(1s), std::future_status::ready);
		ASSERT_TRUE(waitFuture.get());

		auto &values = endpoints.consumer.drain();
		ASSERT_EQ(values.size(), 1u);
		EXPECT_EQ(values.front(), iteration);
	}
}
