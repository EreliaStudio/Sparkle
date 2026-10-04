#include <threading/task.hpp>
#include <threading/task_group.hpp>
#include <threading/worker_pool.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(
	std::is_same_v<
		spk::TaskGroup<int>::TaskAnswer,
		spk::Task<int>::Answer>);
static_assert(
	std::is_same_v<
		spk::TaskGroup<int>::Answer::CompletionContract,
		spk::ContractProvider<>::Contract>);

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

TEST(TaskGroup, WaitBlocksUntilEveryChildIsTerminal)
{
	spk::Task<int> firstTask;
	spk::Task<int> secondTask;

	spk::TaskGroup<int> group;
	group.add(firstTask.answer());
	group.add(secondTask.answer());
	const auto answer = std::move(group).answer();

	std::atomic<bool> waiting = false;
	std::atomic<bool> returned = false;
	std::thread waiter(
		[&] {
			waiting.store(
				true,
				std::memory_order_release);
			answer.wait();
			returned.store(
				true,
				std::memory_order_release);
		});

	while (
		waiting.load(
			std::memory_order_acquire) == false)
	{
		std::this_thread::yield();
	}

	firstTask.validate(1);
	EXPECT_FALSE(
		returned.load(
			std::memory_order_acquire));

	secondTask.validate(2);
	waiter.join();

	EXPECT_TRUE(
		returned.load(
			std::memory_order_acquire));
	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
}

TEST(TaskGroup, GetWaitsAndReturnsAnswers)
{
	spk::Task<int> firstTask;
	spk::Task<int> secondTask;

	spk::TaskGroup<int> group;
	group.add(firstTask.answer());
	group.add(secondTask.answer());
	const auto answer = std::move(group).answer();

	std::thread producer(
		[&] {
			firstTask.validate(7);
			secondTask.validate(8);
		});

	const auto answers = answer.get();
	producer.join();

	ASSERT_EQ(answers.size(), 2u);
	EXPECT_EQ(answers[0u].result(), 7);
	EXPECT_EQ(answers[1u].result(), 8);
}

TEST(TaskGroup, GetRethrowsFirstStoredFailure)
{
	spk::Task<int> failedTask;
	spk::Task<int> completedTask;

	spk::TaskGroup<int> group;
	group.add(failedTask.answer());
	group.add(completedTask.answer());
	const auto answer = std::move(group).answer();

	failedTask.fail(
		std::make_exception_ptr(
			std::runtime_error(
				"group get failure")));
	completedTask.validate(12);

	try
	{
		(void)answer.get();
		FAIL() << "Expected stored group failure";
	} catch (const std::runtime_error &exception)
	{
		EXPECT_STREQ(
			exception.what(),
			"group get failure");
	}
}

TEST(TaskGroup, CompletionWaitsForEveryManuallySettledTask)
{
	spk::Task<int> firstTask;
	spk::Task<int> secondTask;
	auto first = firstTask.answer();
	auto second = secondTask.answer();

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

	firstTask.validate(10);

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Pending);
	EXPECT_EQ(
		calls.load(std::memory_order_relaxed),
		0);

	secondTask.validate(20);

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(answer.at(0u).result(), 10);
	EXPECT_EQ(answer.at(1u).result(), 20);
	EXPECT_EQ(
		calls.load(std::memory_order_relaxed),
		1);
	EXPECT_FALSE(contract.isValid());
}

TEST(TaskGroup, FailureWaitsForEveryChildAndPreservesAnswers)
{
	spk::Task<int> failedTask;
	spk::Task<int> completedTask;
	auto failed = failedTask.answer();
	auto completed = completedTask.answer();

	spk::TaskGroup<int> group;
	group.add(failed);
	group.add(completed);
	auto answer = std::move(group).answer();

	failedTask.fail(
		std::make_exception_ptr(
			std::runtime_error(
				"group child failure")));

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Pending);

	completedTask.validate(31);

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
	spk::Task<int> firstTask;
	spk::Task<int> secondTask;
	firstTask.validate(5);
	secondTask.validate(6);

	spk::TaskGroup<int> group;
	group.add(firstTask.answer());
	group.add(secondTask.answer());
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

TEST(TaskGroup, AcceptsWorkerPoolProducedAnswers)
{
	spk::WorkerPool workerPool(2u);

	auto first = workerPool.submit(
		[] {
			return 11;
		});
	auto second = workerPool.submit(
		[] {
			return 12;
		});

	spk::TaskGroup<int> group;
	group.add(first);
	group.add(second);
	auto answer = std::move(group).answer();

	answer.wait();

	ASSERT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(answer.at(0u).result(), 11);
	EXPECT_EQ(answer.at(1u).result(), 12);
}

TEST(TaskGroup, ManualAndWorkerAnswersCanShareOneGroup)
{
	spk::WorkerPool workerPool(1u);
	spk::Task<int> manualTask;

	auto workerAnswer = workerPool.submit(
		[] {
			return 23;
		});

	spk::TaskGroup<int> group;
	group.add(manualTask.answer());
	group.add(workerAnswer);
	auto answer = std::move(group).answer();

	manualTask.validate(22);

	answer.wait();

	ASSERT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(answer.at(0u).result(), 22);
	EXPECT_EQ(answer.at(1u).result(), 23);
}

TEST(TaskGroup, ConcurrentChildCompletionNotifiesOnce)
{
	constexpr int TaskCount = 32;

	std::vector<spk::Task<int>> tasks;
	tasks.reserve(TaskCount);

	spk::TaskGroup<int> group;
	for (int index = 0; index < TaskCount; ++index)
	{
		tasks.emplace_back();
		group.add(tasks.back().answer());
	}

	auto answer = std::move(group).answer();
	std::atomic<int> calls = 0;
	auto contract = answer.subscribeToCompletion(
		[&calls] {
			calls.fetch_add(
				1,
				std::memory_order_relaxed);
		});

	std::vector<std::jthread> threads;
	threads.reserve(TaskCount);
	for (int index = 0; index < TaskCount; ++index)
	{
		threads.emplace_back(
			[&task = tasks[static_cast<std::size_t>(index)],
			 index] {
				task.validate(index);
			});
	}
	threads.clear();

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(
		calls.load(std::memory_order_relaxed),
		1);
	EXPECT_FALSE(contract.isValid());
}

TEST(TaskGroup, AnswerAtRejectsOutOfRangeIndex)
{
	spk::Task<int> task;
	task.validate(4);

	spk::TaskGroup<int> group;
	group.add(task.answer());
	auto answer = std::move(group).answer();

	EXPECT_THROW((void)answer.at(1u), spk::Exception);
}
