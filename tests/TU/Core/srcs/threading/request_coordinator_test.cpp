#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <stdexcept>

#include "threading/request_coordinator.hpp"

namespace
{
	using namespace std::chrono_literals;
	using Coordinator = spk::RequestCoordinator<int, int>;
	using Duration = Coordinator::Duration;
	using Answer = spk::Task<int>::Answer;

	Coordinator::RetryPolicy fixedRetry()
	{
		return [](int, Coordinator::Failure) -> std::optional<Duration> {
			return 5ms;
		};
	}
}

TEST(RequestCoordinatorTest, DeduplicatesAndPublishesOnlyCollectedResults)
{
	Coordinator coordinator(2, 10ms);
	spk::Task<int> task;
	EXPECT_TRUE(coordinator.request(4, 0ms, [&]() {
		return task.answer();
	}));
	EXPECT_FALSE(coordinator.request(4, 1ms, [&]() {
		return task.answer();
	}));

	int results = 0;
	task.validate(12);
	coordinator.collect(2ms, [&](int key, int result) {
		EXPECT_EQ(key, 4);
		results = result;
	});
	EXPECT_EQ(results, 12);
	EXPECT_EQ(coordinator.inFlightCount(), 0u);
}

TEST(RequestCoordinatorTest, RetryAfterTimeoutDoesNotReleasePhysicalBudgetPrematurely)
{
	Coordinator coordinator(1, 10ms, fixedRetry());
	spk::Task<int> expired;
	spk::Task<int> replacement;
	ASSERT_TRUE(coordinator.request(1, 0ms, [&]() {
		return expired.answer();
	}));

	coordinator.advance(10ms);
	coordinator.collect(10ms, [](int, int) {
		FAIL() << "Expired answer must not publish";
	});
	EXPECT_EQ(coordinator.inFlightCount(), 1u);
	EXPECT_FALSE(coordinator.request(1, 15ms, [&]() {
		return replacement.answer();
	}));

	expired.validate(100);
	coordinator.collect(16ms, [](int, int) {
		FAIL() << "Expired answer must not publish";
	});
	EXPECT_EQ(coordinator.inFlightCount(), 0u);
	EXPECT_TRUE(coordinator.request(1, 16ms, [&]() {
		return replacement.answer();
	}));

	int published = 0;
	replacement.validate(200);
	coordinator.collect(17ms, [&](int, int value) {
		published = value;
	});
	EXPECT_EQ(published, 200);
}

TEST(RequestCoordinatorTest, RetryMayOverlapAnExpiredWorkerWhenBudgetAllows)
{
	Coordinator coordinator(2, 10ms, fixedRetry());
	spk::Task<int> previous;
	spk::Task<int> current;
	EXPECT_TRUE(coordinator.request(3, 0ms, [&]() {
		return previous.answer();
	}));
	coordinator.advance(10ms);
	coordinator.collect(10ms, [](int, int) {
		FAIL();
	});
	EXPECT_FALSE(coordinator.request(3, 14ms, [&]() {
		return current.answer();
	}));
	EXPECT_TRUE(coordinator.request(3, 15ms, [&]() {
		return current.answer();
	}));
	EXPECT_EQ(coordinator.inFlightCount(), 2u);

	previous.validate(1);
	coordinator.collect(16ms, [](int, int) {
		FAIL() << "Stale answer must not publish";
	});
	int received = 0;
	current.validate(2);
	coordinator.collect(17ms, [&](int key, int value) {
		EXPECT_EQ(key, 3);
		received = value;
	});
	EXPECT_EQ(received, 2);
	EXPECT_EQ(coordinator.inFlightCount(), 0u);
}

TEST(RequestCoordinatorTest, FailedAndSubmissionFailuresRespectRetryPolicy)
{
	Coordinator coordinator(1, 20ms, fixedRetry());
	spk::Task<int> failed;
	EXPECT_TRUE(coordinator.request(2, 0ms, [&]() {
		return failed.answer();
	}));
	failed.fail(std::make_exception_ptr(std::runtime_error("failed")));
	coordinator.collect(1ms, [](int, int) {
		FAIL();
	});

	spk::Task<int> next;
	EXPECT_FALSE(coordinator.request(2, 5ms, [&]() {
		return next.answer();
	}));
	EXPECT_TRUE(coordinator.request(2, 6ms, [&]() {
		return next.answer();
	}));
	next.validate(7);
	coordinator.collect(7ms, [](int key, int result) {
		EXPECT_EQ(key, 2);
		EXPECT_EQ(result, 7);
	});

	EXPECT_FALSE(coordinator.request(9, 8ms, []() -> Answer {
		throw std::runtime_error("synchronous submission error");
	}));
	EXPECT_EQ(coordinator.inFlightCount(), 0u);
	EXPECT_FALSE(coordinator.request(9, 12ms, [&]() {
		return next.answer();
	}));
}

TEST(RequestCoordinatorTest, AbsentRetryPolicyPreventsAutomaticRetry)
{
	Coordinator coordinator(1, 10ms);
	spk::Task<int> failed;
	EXPECT_TRUE(coordinator.request(1, 0ms, [&]() {
		return failed.answer();
	}));
	failed.fail(std::make_exception_ptr(std::runtime_error("failed")));
	coordinator.collect(1ms, [](int, int) {
		FAIL();
	});
	spk::Task<int> replacement;
	EXPECT_FALSE(coordinator.request(1, 100ms, [&]() {
		return replacement.answer();
	}));
	coordinator.invalidate(1);
	EXPECT_TRUE(coordinator.request(1, 101ms, [&]() {
		return replacement.answer();
	}));
}

TEST(RequestCoordinatorTest, InvalidationDuringSubmissionCannotReviveStaleRequest)
{
	Coordinator coordinator(2, 10ms, fixedRetry());
	spk::Task<int> stale;
	spk::Task<int> current;
	EXPECT_TRUE(coordinator.request(1, 0ms, [&]() {
		coordinator.invalidate(1);
		return stale.answer();
	}));
	EXPECT_EQ(coordinator.inFlightCount(), 1u);
	EXPECT_TRUE(coordinator.request(1, 1ms, [&]() {
		return current.answer();
	}));

	stale.validate(3);
	current.validate(4);
	int received = 0;
	coordinator.collect(2ms, [&](int, int value) {
		received = value;
	});
	EXPECT_EQ(received, 4);
	EXPECT_EQ(coordinator.inFlightCount(), 0u);
}

TEST(RequestCoordinatorTest, OptionalEmptyResultIsAValidCompletion)
{
	spk::RequestCoordinator<int, std::optional<int>> coordinator(1, 10ms);
	spk::Task<std::optional<int>> task;
	EXPECT_TRUE(coordinator.request(8, 0ms, [&]() {
		return task.answer();
	}));
	task.validate(std::nullopt);
	bool absent = false;
	coordinator.collect(1ms, [&](int key, const std::optional<int> &value) {
		EXPECT_EQ(key, 8);
		absent = !value.has_value();
	});
	EXPECT_TRUE(absent);
}
