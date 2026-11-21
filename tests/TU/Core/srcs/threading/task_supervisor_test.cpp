#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>

#include "core/platform/controlled_timer.hpp"
#include "threading/task_supervisor.hpp"

namespace
{
	using namespace std::chrono_literals;
}

TEST(TaskSupervisorTest, CollectsCompletedTaskOnCallingThreadOnly)
{
	spk::Task<int> task;
	spk::TaskSupervisor<int> supervisor;
	const auto id = supervisor.watch(task.answer(), std::make_unique<spk::ControlledTimer>(10ms));
	const std::thread::id owner = std::this_thread::get_id();
	std::thread::id callback;
	int result = 0;

	std::thread worker([&task]() {
		task.validate(42);
	});
	worker.join();
	EXPECT_EQ(supervisor.inFlightCount(), 1u);
	supervisor.collect([&](auto receivedId, const auto &answer) {
		EXPECT_EQ(receivedId, id);
		callback = std::this_thread::get_id();
		result = answer.result();
	});
	EXPECT_EQ(callback, owner);
	EXPECT_EQ(result, 42);
	EXPECT_EQ(supervisor.inFlightCount(), 0u);
}

TEST(TaskSupervisorTest, TimeoutFiresOnceAndLateAnswerStillOccupiesPhysicalSlot)
{
	spk::Task<int> task;
	spk::TaskSupervisor<int> supervisor;
	auto timer = std::make_unique<spk::ControlledTimer>(10ms);
	auto *clock = timer.get();
	int timeouts = 0;
	int results = 0;
	const auto id = supervisor.watch(task.answer(), std::move(timer));
	clock->advance(10ms);
	supervisor.collect(
		[&](auto, const auto &) {
			++results;
		},
		[&](auto timedOutId) {
			EXPECT_EQ(timedOutId, id);
			++timeouts;
		});
	EXPECT_EQ(timeouts, 1);
	EXPECT_EQ(supervisor.inFlightCount(), 1u);

	supervisor.collect([&](auto, const auto &) {
		++results;
	});
	EXPECT_EQ(timeouts, 1);
	supervisor.invalidate(id);
	task.validate(99);
	supervisor.collect([&](auto, const auto &) {
		++results;
	});
	EXPECT_EQ(supervisor.inFlightCount(), 0u);
	EXPECT_EQ(results, 0);
}

TEST(TaskSupervisorTest, FailedTaskIsCollectedWithoutBlocking)
{
	spk::Task<int> task;
	spk::TaskSupervisor<int> supervisor;
	(void)supervisor.watch(task.answer(), std::make_unique<spk::ControlledTimer>(10ms));
	task.fail(std::make_exception_ptr(std::runtime_error("task error")));
	int failures = 0;
	supervisor.collect([&](auto, const auto &answer) {
		EXPECT_EQ(answer.status(), spk::Task<int>::Status::Failed);
		EXPECT_THROW(std::rethrow_exception(answer.failure()), std::runtime_error);
		++failures;
	});
	EXPECT_EQ(failures, 1);
	EXPECT_EQ(supervisor.inFlightCount(), 0u);
}

TEST(TaskSupervisorTest, InvalidationAndReentrantWatchNeverPublishesStaleAnswer)
{
	spk::Task<int> first;
	spk::Task<int> second;
	spk::Task<int> third;
	spk::TaskSupervisor<int> supervisor;
	const auto firstId = supervisor.watch(first.answer(), std::make_unique<spk::ControlledTimer>(10ms));
	(void)supervisor.watch(second.answer(), std::make_unique<spk::ControlledTimer>(10ms));
	supervisor.invalidate(firstId);
	first.validate(10);
	second.validate(20);
	third.validate(30);

	int calls = 0;
	supervisor.collect([&](auto, const auto &) {
		++calls;
		(void)supervisor.watch(third.answer(), std::make_unique<spk::ControlledTimer>(10ms));
	});
	EXPECT_EQ(calls, 1);
	EXPECT_EQ(supervisor.inFlightCount(), 1u);
	supervisor.collect([&](auto, const auto &) {
		++calls;
	});
	EXPECT_EQ(calls, 2);
	EXPECT_EQ(supervisor.inFlightCount(), 0u);
}
