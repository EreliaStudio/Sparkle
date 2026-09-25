#include <threading/task.hpp>
#include <threading/task_group.hpp>
#include <threading/worker_pool.hpp>

#include <exception.hpp>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>
#include <utility>

namespace
{
	template <typename TAnswer>
	bool waitUntilSettled(const TAnswer &answer)
	{
		const auto deadline =
			std::chrono::steady_clock::now() +
			std::chrono::seconds(5);

		while (
			answer.status() ==
				spk::Task<int>::Status::Pending &&
			std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::yield();
		}

		return answer.status() !=
			   spk::Task<int>::Status::Pending;
	}
}

TEST(TaskCompletion, SubscriberRunsOnceWhenTaskSettles)
{
	spk::WorkerPool workerPool(1u);
	std::promise<void> release;
	const auto gate = release.get_future().share();
	std::atomic<int> calls = 0;

	auto answer = workerPool.submit(
		spk::Task<int>(
			[gate] {
				gate.wait();
				return 42;
			}));

	auto contract = answer.subscribeToCompletion(
		[&calls] {
			calls.fetch_add(
				1,
				std::memory_order_relaxed);
		});

	release.set_value();

	ASSERT_TRUE(waitUntilSettled(answer));
	EXPECT_EQ(
		calls.load(std::memory_order_relaxed),
		1);
	EXPECT_FALSE(contract.isValid());
}

TEST(TaskCompletion, SubscriberAddedAfterCompletionRunsImmediately)
{
	spk::WorkerPool workerPool(1u);
	auto answer = workerPool.submit(
		spk::Task<int>([] {
			return 17;
		}));

	ASSERT_TRUE(waitUntilSettled(answer));

	int calls = 0;
	auto contract = answer.subscribeToCompletion(
		[&calls] {
			++calls;
		});

	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(contract.isValid());
}

TEST(TaskCompletion, ResignedSubscriberIsNotCalled)
{
	spk::WorkerPool workerPool(1u);
	std::promise<void> release;
	const auto gate = release.get_future().share();
	std::atomic<int> calls = 0;

	auto answer = workerPool.submit(
		spk::Task<int>(
			[gate] {
				gate.wait();
				return 1;
			}));

	auto contract = answer.subscribeToCompletion(
		[&calls] {
			calls.fetch_add(
				1,
				std::memory_order_relaxed);
		});
	contract.resign();

	release.set_value();

	ASSERT_TRUE(waitUntilSettled(answer));
	EXPECT_EQ(
		calls.load(std::memory_order_relaxed),
		0);
}

TEST(TaskCompletion, ConcurrentSubscribeAndCompletionNeverLosesNotification)
{
	spk::WorkerPool workerPool(4u);

	for (int iteration = 0; iteration < 256; ++iteration)
	{
		std::promise<void> release;
		const auto gate = release.get_future().share();
		std::atomic<int> calls = 0;

		auto answer = workerPool.submit(
			spk::Task<int>(
				[gate] {
					gate.wait();
					return 9;
				}));

		std::thread subscriber(
			[&] {
				auto contract =
					answer.subscribeToCompletion(
						[&calls] {
							calls.fetch_add(
								1,
								std::memory_order_relaxed);
						});

				while (
					answer.status() ==
					spk::Task<int>::Status::Pending)
				{
					std::this_thread::yield();
				}
			});

		release.set_value();
		subscriber.join();

		ASSERT_TRUE(waitUntilSettled(answer));
		EXPECT_EQ(
			calls.load(std::memory_order_relaxed),
			1);
	}
}

TEST(TaskGroup, EmptyGroupIsImmediatelyCompleted)
{
	spk::TaskGroup<int> group;
	auto answer = std::move(group).answer();

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(answer.size(), 0u);
	EXPECT_TRUE(answer.answers().empty());
}

TEST(TaskGroup, CompletionWaitsForEveryChild)
{
	spk::WorkerPool workerPool(2u);
	std::promise<void> firstRelease;
	std::promise<void> secondRelease;
	const auto firstGate =
		firstRelease.get_future().share();
	const auto secondGate =
		secondRelease.get_future().share();

	auto first = workerPool.submit(
		spk::Task<int>(
			[firstGate] {
				firstGate.wait();
				return 10;
			}));
	auto second = workerPool.submit(
		spk::Task<int>(
			[secondGate] {
				secondGate.wait();
				return 20;
			}));

	spk::TaskGroup<int> group;
	group.add(first);
	group.add(second);
	auto answer = std::move(group).answer();

	std::atomic<int> calls = 0;
	auto contract = answer.subscribeToCompletion(
		[&calls] {
			calls.fetch_add(
				1,
				std::memory_order_relaxed);
		});

	firstRelease.set_value();
	ASSERT_TRUE(waitUntilSettled(first));
	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Pending);
	EXPECT_EQ(
		calls.load(std::memory_order_relaxed),
		0);

	secondRelease.set_value();
	ASSERT_TRUE(waitUntilSettled(answer));
	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(
		calls.load(std::memory_order_relaxed),
		1);
	EXPECT_FALSE(contract.isValid());
}

TEST(TaskGroup, FailureWaitsForEveryChildAndPreservesAnswers)
{
	spk::WorkerPool workerPool(2u);
	std::promise<void> release;
	const auto gate = release.get_future().share();

	auto failed = workerPool.submit(
		spk::Task<int>(
			[]() -> int {
				throw std::runtime_error(
					"group child failure");
			}));
	auto completed = workerPool.submit(
		spk::Task<int>(
			[gate] {
				gate.wait();
				return 31;
			}));

	spk::TaskGroup<int> group;
	group.add(failed);
	group.add(completed);
	auto answer = std::move(group).answer();

	ASSERT_TRUE(waitUntilSettled(failed));
	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Pending);

	release.set_value();
	ASSERT_TRUE(waitUntilSettled(answer));

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Failed);
	ASSERT_EQ(answer.size(), 2u);
	EXPECT_EQ(
		answer.at(0u).status(),
		spk::Task<int>::Status::Failed);
	EXPECT_EQ(answer.at(1u).result(), 31);
	EXPECT_THROW(
		std::rethrow_exception(
			answer.at(0u).failure()),
		std::runtime_error);
}

TEST(TaskGroup, AlreadySettledChildrenCompleteWhenGroupIsSealed)
{
	spk::WorkerPool workerPool(1u);
	auto first = workerPool.submit(
		spk::Task<int>([] {
			return 5;
		}));
	auto second = workerPool.submit(
		spk::Task<int>([] {
			return 6;
		}));

	ASSERT_TRUE(waitUntilSettled(first));
	ASSERT_TRUE(waitUntilSettled(second));

	spk::TaskGroup<int> group;
	group.add(first);
	group.add(second);
	auto answer = std::move(group).answer();

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(answer.at(0u).result(), 5);
	EXPECT_EQ(answer.at(1u).result(), 6);

	int calls = 0;
	auto contract = answer.subscribeToCompletion(
		[&calls] {
			++calls;
		});

	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(contract.isValid());
}
