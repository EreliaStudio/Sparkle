#include <container/thread_safe_set.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <future>
#include <memory>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
	struct MoveOnlyValue
	{
		int value = 0;

		explicit MoveOnlyValue(int p_value) :
			value(p_value)
		{
		}

		MoveOnlyValue(const MoveOnlyValue &) = delete;
		MoveOnlyValue &operator=(const MoveOnlyValue &) = delete;
		MoveOnlyValue(MoveOnlyValue &&) noexcept = default;
		MoveOnlyValue &operator=(MoveOnlyValue &&) noexcept = default;

		[[nodiscard]] bool operator==(
			const MoveOnlyValue &other) const noexcept
		{
			return value == other.value;
		}
	};

	struct MoveOnlyValueHash
	{
		[[nodiscard]] std::size_t operator()(
			const MoveOnlyValue &value) const noexcept
		{
			return std::hash<int>{}(value.value);
		}
	};

	using MoveOnlySet =
		spk::ThreadSafeSet<MoveOnlyValue, MoveOnlyValueHash>;

	static_assert(
		std::is_copy_constructible_v<
			spk::ThreadSafeSet<int>::Producer>);
	static_assert(
		!std::is_copy_constructible_v<
			spk::ThreadSafeSet<int>::Consumer>);
	static_assert(
		std::is_move_constructible_v<
			spk::ThreadSafeSet<int>::Consumer>);
}

TEST(ThreadSafeSet, PublishReportsWhetherValueWasNew)
{
	spk::ThreadSafeSet<int> values;

	EXPECT_TRUE(values.publish(4));
	EXPECT_FALSE(values.publish(4));
	EXPECT_TRUE(values.emplace(8));
	EXPECT_FALSE(values.emplace(8));
}

TEST(ThreadSafeSet, ContainsAndEraseTrackMembership)
{
	spk::ThreadSafeSet<int> values;

	EXPECT_FALSE(values.contains(4));
	EXPECT_FALSE(values.erase(4));

	EXPECT_TRUE(values.publish(4));
	EXPECT_TRUE(values.contains(4));
	EXPECT_TRUE(values.erase(4));
	EXPECT_FALSE(values.contains(4));
	EXPECT_FALSE(values.erase(4));
}

TEST(ThreadSafeSet, DrainReturnsAllValuesAndEmptiesSharedState)
{
	spk::ThreadSafeSet<int> values;
	spk::ThreadSafeSet<int>::container_type destination = {99};

	EXPECT_TRUE(values.publish(3));
	EXPECT_TRUE(values.publish(7));
	EXPECT_TRUE(values.publish(11));

	const auto &drained = values.drain(destination);

	EXPECT_EQ(&drained, &destination);
	EXPECT_EQ(destination.size(), 3u);
	EXPECT_TRUE(destination.contains(3));
	EXPECT_TRUE(destination.contains(7));
	EXPECT_TRUE(destination.contains(11));
	EXPECT_FALSE(destination.contains(99));

	spk::ThreadSafeSet<int>::container_type empty;
	EXPECT_TRUE(values.drain(empty).empty());
}

TEST(ThreadSafeSet, DrainedValueCanBeRequestedAgain)
{
	spk::ThreadSafeSet<int> values;
	spk::ThreadSafeSet<int>::container_type drained;

	EXPECT_TRUE(values.publish(17));
	(void)values.drain(drained);
	EXPECT_TRUE(values.publish(17));
	EXPECT_FALSE(values.publish(17));
}

TEST(ThreadSafeSet, SupportsMoveOnlyValues)
{
	MoveOnlySet values;
	MoveOnlySet::container_type drained;

	EXPECT_TRUE(values.emplace(5));
	EXPECT_FALSE(values.emplace(5));
	EXPECT_TRUE(values.publish(MoveOnlyValue(9)));

	(void)values.drain(drained);

	EXPECT_EQ(drained.size(), 2u);
	EXPECT_TRUE(drained.contains(MoveOnlyValue(5)));
	EXPECT_TRUE(drained.contains(MoveOnlyValue(9)));
}

TEST(ThreadSafeSet, ProducerAndConsumerShareState)
{
	auto endpoints = spk::ThreadSafeSet<int>::create();

	EXPECT_TRUE(endpoints.producer.publish(21));
	EXPECT_TRUE(endpoints.producer.emplace(34));

	const auto &values = endpoints.consumer.drain();

	EXPECT_EQ(values.size(), 2u);
	EXPECT_TRUE(values.contains(21));
	EXPECT_TRUE(values.contains(34));
}

TEST(ThreadSafeSet, EndpointsKeepStateAliveWithoutOwningContainer)
{
	auto endpoints = [] {
		spk::ThreadSafeSet<int> values;
		return spk::ThreadSafeSet<int>::Endpoints{
			.producer = values.producer(),
			.consumer = values.consumer()};
	}();

	EXPECT_TRUE(endpoints.producer.publish(55));

	const auto &values = endpoints.consumer.drain();
	ASSERT_EQ(values.size(), 1u);
	EXPECT_TRUE(values.contains(55));
}

TEST(ThreadSafeSet, WaitReturnsImmediatelyWhenValueAlreadyExists)
{
	auto endpoints = spk::ThreadSafeSet<int>::create();

	EXPECT_TRUE(endpoints.producer.publish(5));
	EXPECT_TRUE(endpoints.consumer.wait());
}

TEST(ThreadSafeSet, WaitWakesWhenProducerPublishes)
{
	auto endpoints = spk::ThreadSafeSet<int>::create();
	std::promise<void> waiterStarted;
	std::promise<bool> waitResult;
	auto started = waiterStarted.get_future();
	auto result = waitResult.get_future();

	std::jthread waiter(
		[consumer = std::move(endpoints.consumer),
		 &waiterStarted,
		 &waitResult]() mutable {
			waiterStarted.set_value();
			waitResult.set_value(consumer.wait());
		});

	started.wait();
	EXPECT_TRUE(endpoints.producer.publish(42));
	EXPECT_TRUE(result.get());
}

TEST(ThreadSafeSet, StopTokenUnblocksEmptyWait)
{
	auto endpoints = spk::ThreadSafeSet<int>::create();
	std::promise<bool> waitResult;
	auto result = waitResult.get_future();

	std::jthread waiter(
		[consumer = std::move(endpoints.consumer),
		 &waitResult](std::stop_token stopToken) mutable {
			waitResult.set_value(consumer.wait(stopToken));
		});

	waiter.request_stop();
	EXPECT_FALSE(result.get());
}

TEST(ThreadSafeSet, ConcurrentDuplicatePublishCreatesOneValue)
{
	auto endpoints = spk::ThreadSafeSet<int>::create();
	std::vector<std::thread> threads;

	for (std::size_t index = 0; index < 8u; ++index)
	{
		threads.emplace_back(
			[producer = endpoints.producer]() mutable {
				for (int repetition = 0; repetition < 1000; ++repetition)
				{
					(void)producer.publish(7);
				}
			});
	}

	for (std::thread &thread : threads)
	{
		thread.join();
	}

	const auto &values = endpoints.consumer.drain();
	ASSERT_EQ(values.size(), 1u);
	EXPECT_TRUE(values.contains(7));
}

TEST(ThreadSafeSet, MultipleProducersPublishEveryUniqueValueExactlyOnce)
{
	auto endpoints = spk::ThreadSafeSet<int>::create();
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
					const int value =
						producerIndex * ValuesPerProducer +
						valueIndex;
					(void)producer.publish(value);
					(void)producer.publish(value);
				}
			});
	}

	for (std::thread &producer : producers)
	{
		producer.join();
	}

	const auto &values = endpoints.consumer.drain();
	ASSERT_EQ(
		values.size(),
		static_cast<std::size_t>(
			ProducerCount * ValuesPerProducer));

	for (int value = 0;
		 value < ProducerCount * ValuesPerProducer;
		 ++value)
	{
		EXPECT_TRUE(values.contains(value));
	}
}

TEST(ThreadSafeSet, ConcurrentConsumersDrainSharedStateOnlyOnce)
{
	spk::ThreadSafeSet<int> values;
	for (int value = 0; value < 256; ++value)
	{
		(void)values.publish(value);
	}

	auto firstConsumer = values.consumer();
	auto secondConsumer = values.consumer();
	std::atomic<std::size_t> drainedCount = 0u;

	std::thread first(
		[consumer = std::move(firstConsumer),
		 &drainedCount]() mutable {
			drainedCount.fetch_add(
				consumer.drain().size(),
				std::memory_order_relaxed);
		});
	std::thread second(
		[consumer = std::move(secondConsumer),
		 &drainedCount]() mutable {
			drainedCount.fetch_add(
				consumer.drain().size(),
				std::memory_order_relaxed);
		});

	first.join();
	second.join();

	EXPECT_EQ(
		drainedCount.load(std::memory_order_relaxed),
		256u);
}
