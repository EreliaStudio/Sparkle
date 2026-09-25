#include <threading/task.hpp>
#include <threading/worker_pool.hpp>

#include <exception.hpp>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
	template <typename TResult>
	bool waitUntilSettled(
		const typename spk::Task<TResult>::Answer &answer)
	{
		const auto deadline =
			std::chrono::steady_clock::now() +
			std::chrono::seconds(5);

		while (
			answer.status() == spk::Task<TResult>::Status::Pending &&
			std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::yield();
		}

		return answer.status() !=
			   spk::Task<TResult>::Status::Pending;
	}

	bool waitUntilAtLeast(
		const std::atomic<int> &value,
		int expected)
	{
		const auto deadline =
			std::chrono::steady_clock::now() +
			std::chrono::seconds(5);

		while (
			value.load(std::memory_order_acquire) < expected &&
			std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::yield();
		}

		return value.load(std::memory_order_acquire) >= expected;
	}

	struct MoveOnlyResult
	{
		std::unique_ptr<int> value;

		explicit MoveOnlyResult(int p_value) :
			value(std::make_unique<int>(p_value))
		{
		}

		MoveOnlyResult(const MoveOnlyResult &) = delete;
		MoveOnlyResult &operator=(const MoveOnlyResult &) = delete;
		MoveOnlyResult(MoveOnlyResult &&) noexcept = default;
		MoveOnlyResult &operator=(MoveOnlyResult &&) noexcept = default;
	};

	static_assert(!std::is_copy_constructible_v<spk::Task<int>>);
	static_assert(std::is_move_constructible_v<spk::Task<int>>);
	static_assert(
		std::is_copy_constructible_v<
			spk::Task<int>::Answer>);
	static_assert(!std::is_copy_constructible_v<spk::WorkerPool>);
	static_assert(!std::is_move_constructible_v<spk::WorkerPool>);
}

TEST(Task, NewAnswerStartsPending)
{
	spk::Task<int> task([] {
		return 1;
	});
	const auto answer = task.answer();

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Pending);
	EXPECT_THROW((void)answer.result(), spk::Exception);
	EXPECT_THROW((void)answer.failure(), spk::Exception);
}

TEST(Task, MultipleAnswersObserveSameCompletion)
{
	spk::WorkerPool workerPool(1u);
	std::promise<void> release;
	const auto gate = release.get_future().share();

	spk::Task<int> task(
		[gate] {
			gate.wait();
			return 42;
		});

	auto first = task.answer();
	auto second = first;
	auto submitted = workerPool.submit(std::move(task));

	EXPECT_EQ(first.status(), spk::Task<int>::Status::Pending);
	EXPECT_EQ(second.status(), spk::Task<int>::Status::Pending);
	EXPECT_EQ(
		submitted.status(),
		spk::Task<int>::Status::Pending);

	release.set_value();

	ASSERT_TRUE(waitUntilSettled<int>(first));
	EXPECT_EQ(first.result(), 42);
	EXPECT_EQ(second.result(), 42);
	EXPECT_EQ(submitted.result(), 42);
}

TEST(Task, SupportsMoveOnlyCallableCapture)
{
	spk::WorkerPool workerPool(1u);
	auto captured = std::make_unique<int>(67);

	spk::Task<int> task(
		[value = std::move(captured)] {
			return *value;
		});

	auto answer = workerPool.submit(std::move(task));

	ASSERT_TRUE(waitUntilSettled<int>(answer));
	EXPECT_EQ(answer.result(), 67);
}

TEST(Task, SupportsMoveOnlyResult)
{
	spk::WorkerPool workerPool(1u);
	spk::Task<MoveOnlyResult> task([] {
		return MoveOnlyResult(83);
	});

	auto answer = workerPool.submit(std::move(task));

	ASSERT_TRUE(waitUntilSettled<MoveOnlyResult>(answer));
	ASSERT_NE(answer.result().value, nullptr);
	EXPECT_EQ(*answer.result().value, 83);
}

TEST(Task, CompletedAnswerRejectsFailureAccess)
{
	spk::WorkerPool workerPool(1u);
	auto answer = workerPool.submit(
		spk::Task<int>([] {
			return 5;
		}));

	ASSERT_TRUE(waitUntilSettled<int>(answer));
	ASSERT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(answer.result(), 5);
	EXPECT_THROW((void)answer.failure(), spk::Exception);
}

TEST(Task, EscapingSpkExceptionProducesFailedAnswer)
{
	spk::WorkerPool workerPool(1u);
	auto answer = workerPool.submit(
		spk::Task<int>(
			[]() -> int {
				throw spk::Exception(
					"expected task failure");
			}));

	ASSERT_TRUE(waitUntilSettled<int>(answer));
	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Failed);
	EXPECT_THROW((void)answer.result(), spk::Exception);

	try
	{
		std::rethrow_exception(answer.failure());
		FAIL() << "Expected stored task failure";
	} catch (const spk::Exception &exception)
	{
		EXPECT_NE(
			std::string(exception.what()).find("expected task failure"),
			std::string::npos);
	}
}

TEST(Task, EscapingStandardExceptionPreservesDynamicType)
{
	spk::WorkerPool workerPool(1u);
	auto answer = workerPool.submit(
		spk::Task<int>(
			[]() -> int {
				throw std::runtime_error(
					"standard failure");
			}));

	ASSERT_TRUE(waitUntilSettled<int>(answer));
	ASSERT_EQ(
		answer.status(),
		spk::Task<int>::Status::Failed);

	EXPECT_THROW(
		std::rethrow_exception(answer.failure()),
		std::runtime_error);
}

TEST(Task, FailedAnswerRejectsResultAccess)
{
	spk::WorkerPool workerPool(1u);
	auto answer = workerPool.submit(
		spk::Task<int>(
			[]() -> int {
				throw std::runtime_error("failure");
			}));

	ASSERT_TRUE(waitUntilSettled<int>(answer));
	EXPECT_THROW((void)answer.result(), spk::Exception);
	EXPECT_NO_THROW((void)answer.failure());
}

TEST(WorkerPool, RejectsZeroWorkers)
{
	EXPECT_THROW((void)spk::WorkerPool(0u), spk::Exception);
}

TEST(WorkerPool, ExplicitWorkerCountIsReported)
{
	spk::WorkerPool workerPool(3u);
	EXPECT_EQ(workerPool.workerCount(), 3u);
}

TEST(WorkerPool, DefaultWorkerCountIsNeverZero)
{
	spk::WorkerPool workerPool;
	EXPECT_GE(workerPool.workerCount(), 1u);
}

TEST(WorkerPool, SingleWorkerExecutesTasksInSubmissionOrder)
{
	spk::WorkerPool workerPool(1u);
	std::mutex orderMutex;
	std::vector<int> order;
	std::vector<spk::Task<int>::Answer> answers;

	for (int index = 0; index < 64; ++index)
	{
		answers.push_back(
			workerPool.submit(
				spk::Task<int>(
					[index, &order, &orderMutex] {
						{
							const std::scoped_lock lock(
								orderMutex);
							order.push_back(index);
						}
						return index;
					})));
	}

	for (const auto &answer : answers)
	{
		ASSERT_TRUE(waitUntilSettled<int>(answer));
	}

	ASSERT_EQ(order.size(), 64u);
	for (int index = 0; index < 64; ++index)
	{
		EXPECT_EQ(
			order[static_cast<std::size_t>(index)],
			index);
	}
}

TEST(WorkerPool, MultipleWorkersCanExecuteTasksConcurrently)
{
	spk::WorkerPool workerPool(2u);
	std::atomic<int> started = 0;
	std::promise<void> release;
	const auto gate = release.get_future().share();

	auto first = workerPool.submit(
		spk::Task<int>(
			[&started, gate] {
				started.fetch_add(
					1,
					std::memory_order_release);
				gate.wait();
				return 1;
			}));
	auto second = workerPool.submit(
		spk::Task<int>(
			[&started, gate] {
				started.fetch_add(
					1,
					std::memory_order_release);
				gate.wait();
				return 2;
			}));

	const bool bothStarted = waitUntilAtLeast(started, 2);
	release.set_value();

	EXPECT_TRUE(bothStarted);
	ASSERT_TRUE(waitUntilSettled<int>(first));
	ASSERT_TRUE(waitUntilSettled<int>(second));
	EXPECT_EQ(first.result(), 1);
	EXPECT_EQ(second.result(), 2);
}

TEST(WorkerPool, HighContentionExecutesEveryTaskExactlyOnce)
{
	constexpr int TaskCount = 2000;
	spk::WorkerPool workerPool(8u);
	std::atomic<int> executionCount = 0;
	std::atomic<long long> executionSum = 0;
	std::vector<spk::Task<int>::Answer> answers;
	answers.reserve(TaskCount);

	for (int index = 0; index < TaskCount; ++index)
	{
		answers.push_back(
			workerPool.submit(
				spk::Task<int>(
					[index,
					 &executionCount,
					 &executionSum] {
						executionCount.fetch_add(
							1,
							std::memory_order_relaxed);
						executionSum.fetch_add(
							index,
							std::memory_order_relaxed);
						return index;
					})));
	}

	for (int index = 0; index < TaskCount; ++index)
	{
		const auto &answer =
			answers[static_cast<std::size_t>(index)];
		ASSERT_TRUE(waitUntilSettled<int>(answer));
		EXPECT_EQ(answer.result(), index);
	}

	EXPECT_EQ(
		executionCount.load(std::memory_order_relaxed),
		TaskCount);
	EXPECT_EQ(
		executionSum.load(std::memory_order_relaxed),
		static_cast<long long>(TaskCount - 1) *
			TaskCount / 2);
}

TEST(WorkerPool, FailedTaskDoesNotStopWorker)
{
	spk::WorkerPool workerPool(1u);

	auto failed = workerPool.submit(
		spk::Task<int>(
			[]() -> int {
				throw std::runtime_error("failure");
			}));
	auto completed = workerPool.submit(
		spk::Task<int>([] {
			return 99;
		}));

	ASSERT_TRUE(waitUntilSettled<int>(failed));
	ASSERT_TRUE(waitUntilSettled<int>(completed));
	EXPECT_EQ(
		failed.status(),
		spk::Task<int>::Status::Failed);
	EXPECT_EQ(
		completed.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(completed.result(), 99);
}

TEST(WorkerPool, DifferentTaskResultTypesShareSamePool)
{
	spk::WorkerPool workerPool(3u);

	auto integerAnswer = workerPool.submit(
		spk::Task<int>([] {
			return 12;
		}));
	auto stringAnswer = workerPool.submit(
		spk::Task<std::string>([] {
			return std::string("worker");
		}));
	auto moveOnlyAnswer = workerPool.submit(
		spk::Task<MoveOnlyResult>([] {
			return MoveOnlyResult(44);
		}));

	ASSERT_TRUE(waitUntilSettled<int>(integerAnswer));
	ASSERT_TRUE(
		waitUntilSettled<std::string>(stringAnswer));
	ASSERT_TRUE(
		waitUntilSettled<MoveOnlyResult>(
			moveOnlyAnswer));

	EXPECT_EQ(integerAnswer.result(), 12);
	EXPECT_EQ(stringAnswer.result(), "worker");
	EXPECT_EQ(*moveOnlyAnswer.result().value, 44);
}

TEST(WorkerPool, DestructionDrainsQueuedJobs)
{
	constexpr int TaskCount = 256;
	std::atomic<int> executionCount = 0;
	std::vector<spk::Task<int>::Answer> answers;
	answers.reserve(TaskCount);

	{
		spk::WorkerPool workerPool(1u);

		for (int index = 0; index < TaskCount; ++index)
		{
			answers.push_back(
				workerPool.submit(
					spk::Task<int>(
						[index, &executionCount] {
							executionCount.fetch_add(
								1,
								std::memory_order_relaxed);
							return index;
						})));
		}
	}

	EXPECT_EQ(
		executionCount.load(std::memory_order_relaxed),
		TaskCount);

	for (int index = 0; index < TaskCount; ++index)
	{
		const auto &answer =
			answers[static_cast<std::size_t>(index)];
		EXPECT_EQ(
			answer.status(),
			spk::Task<int>::Status::Completed);
		EXPECT_EQ(answer.result(), index);
	}
}

TEST(WorkerPool, AnswersRemainValidAfterPoolDestruction)
{
	spk::Task<std::string>::Answer answer = [&] {
		spk::WorkerPool workerPool(1u);
		return workerPool.submit(
			spk::Task<std::string>([] {
				return std::string("persistent answer");
			}));
	}();

	EXPECT_EQ(
		answer.status(),
		spk::Task<std::string>::Status::Completed);
	EXPECT_EQ(answer.result(), "persistent answer");
}
