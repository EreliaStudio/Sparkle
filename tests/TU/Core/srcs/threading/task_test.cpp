#include <threading/task.hpp>

#include <exception.hpp>
#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

namespace
{
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
	static_assert(
		std::is_same_v<
			spk::Task<int>::Answer::CompletionContract,
			spk::ContractProvider<>::Contract>);
}

TEST(Task, NewAnswerStartsPending)
{
	spk::Task<int> task;
	const auto answer = task.answer();

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Pending);
	EXPECT_THROW((void)answer.result(), spk::Exception);
	EXPECT_THROW((void)answer.failure(), spk::Exception);
}

TEST(Task, ValidateCompletesTaskAndPublishesResult)
{
	spk::Task<int> task;
	const auto answer = task.answer();

	task.validate(42);

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Completed);
	EXPECT_EQ(answer.result(), 42);
	EXPECT_THROW((void)answer.failure(), spk::Exception);
}

TEST(Task, FailStoresExceptionAndPublishesFailure)
{
	spk::Task<int> task;
	const auto answer = task.answer();

	task.fail(
		std::make_exception_ptr(
			std::runtime_error("manual failure")));

	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Failed);
	EXPECT_THROW((void)answer.result(), spk::Exception);

	try
	{
		std::rethrow_exception(answer.failure());
		FAIL() << "Expected stored task failure";
	} catch (const std::runtime_error &exception)
	{
		EXPECT_STREQ(exception.what(), "manual failure");
	}
}

TEST(Task, MultipleAnswersObserveSameCompletion)
{
	spk::Task<int> task;
	auto first = task.answer();
	auto second = first;

	task.validate(67);

	EXPECT_EQ(first.result(), 67);
	EXPECT_EQ(second.result(), 67);
}

TEST(Task, SupportsMoveOnlyResult)
{
	spk::Task<MoveOnlyResult> task;
	auto answer = task.answer();

	task.validate(MoveOnlyResult(83));

	ASSERT_EQ(
		answer.status(),
		spk::Task<MoveOnlyResult>::Status::Completed);
	ASSERT_NE(answer.result().value, nullptr);
	EXPECT_EQ(*answer.result().value, 83);
}

TEST(Task, AnswerKeepsCompletedStateAliveAfterTaskDestruction)
{
	spk::Task<std::string>::Answer answer = [] {
		spk::Task<std::string> task;
		auto result = task.answer();
		task.validate("persistent result");
		return result;
	}();

	EXPECT_EQ(
		answer.status(),
		spk::Task<std::string>::Status::Completed);
	EXPECT_EQ(answer.result(), "persistent result");
}

TEST(Task, SubscriberRunsOnceWhenTaskIsValidated)
{
	spk::Task<int> task;
	auto answer = task.answer();
	int calls = 0;

	auto contract = answer.subscribeToCompletion(
		[&calls] {
			++calls;
		});

	task.validate(5);

	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(contract.isValid());
}

TEST(Task, SubscriberRunsOnceWhenTaskFails)
{
	spk::Task<int> task;
	auto answer = task.answer();
	int calls = 0;

	auto contract = answer.subscribeToCompletion(
		[&calls] {
			++calls;
		});

	task.fail(
		std::make_exception_ptr(
			std::runtime_error("failure")));

	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(contract.isValid());
}

TEST(Task, SubscriberAddedAfterCompletionRunsImmediately)
{
	spk::Task<int> task;
	auto answer = task.answer();
	task.validate(17);

	int calls = 0;
	auto contract = answer.subscribeToCompletion(
		[&calls] {
			++calls;
		});

	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(contract.isValid());
}

TEST(Task, ResignedSubscriberIsNotCalled)
{
	spk::Task<int> task;
	auto answer = task.answer();
	int calls = 0;

	auto contract = answer.subscribeToCompletion(
		[&calls] {
			++calls;
		});
	contract.resign();

	task.validate(1);

	EXPECT_EQ(calls, 0);
}

TEST(Task, ThrowingSubscriberDoesNotSuppressOtherSubscribers)
{
	spk::Task<int> task;
	auto answer = task.answer();
	int calls = 0;

	auto throwing = answer.subscribeToCompletion(
		[] {
			throw std::runtime_error(
				"completion callback failure");
		});
	auto observing = answer.subscribeToCompletion(
		[&calls] {
			++calls;
		});

	EXPECT_NO_THROW(task.validate(1));
	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(throwing.isValid());
	EXPECT_FALSE(observing.isValid());
}

TEST(Task, RejectsSecondSettlement)
{
	spk::Task<int> completed;
	completed.validate(1);

	EXPECT_THROW(completed.validate(2), spk::Exception);
	EXPECT_THROW(
		completed.fail(
			std::make_exception_ptr(
				std::runtime_error("late failure"))),
		spk::Exception);

	spk::Task<int> failed;
	failed.fail(
		std::make_exception_ptr(
			std::runtime_error("first failure")));

	EXPECT_THROW(failed.validate(3), spk::Exception);
	EXPECT_THROW(
		failed.fail(
			std::make_exception_ptr(
				std::runtime_error("second failure"))),
		spk::Exception);
}

TEST(Task, RejectsNullFailureWithoutSettling)
{
	spk::Task<int> task;
	auto answer = task.answer();

	EXPECT_THROW(task.fail(nullptr), spk::Exception);
	EXPECT_EQ(
		answer.status(),
		spk::Task<int>::Status::Pending);

	task.validate(9);
	EXPECT_EQ(answer.result(), 9);
}

TEST(Task, ConcurrentSubscribeAndCompletionNeverLosesNotification)
{
	for (int iteration = 0; iteration < 256; ++iteration)
	{
		spk::Task<int> task;
		auto answer = task.answer();
		std::atomic<int> calls = 0;

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
					calls.load(
						std::memory_order_acquire) == 0)
				{
					std::this_thread::yield();
				}
			});

		task.validate(iteration);
		subscriber.join();

		EXPECT_EQ(
			calls.load(std::memory_order_relaxed),
			1);
		EXPECT_EQ(answer.result(), iteration);
	}
}
