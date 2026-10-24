#include "network/internal/transfer_completion.hpp"
#include "network/transfer.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <latch>
#include <mutex>
#include <thread>

namespace
{
	using spk::Transfer;
	using spk::NetworkInternal::TransferCompletion;

	bool exerciseConcurrentCompletionQueries()
	{
		constexpr std::size_t SubscriberCount = 8;
		std::jthread watchdog([](std::stop_token stop) {
			std::mutex mutex;
			std::condition_variable_any wake;
			std::unique_lock lock(mutex);
			(void)wake.wait_for(lock, stop, std::chrono::seconds(10), [] {
				return false;
			});
			if (!stop.stop_requested())
			{
				std::_Exit(EXIT_FAILURE);
			}
		});
		Transfer transfer;
		std::latch callbackEntered(1);
		std::latch subscribersStarting(SubscriberCount);
		std::atomic_bool valid = true;
		std::atomic_uint32_t earlyCalls = 0;
		std::array<std::atomic_uint32_t, SubscriberCount> calls{};
		auto early = transfer.subscribeToCompletion([&](Transfer::Status, Transfer::Reason) {
			callbackEntered.count_down();
			subscribersStarting.wait();
			for (std::size_t attempt = 0; attempt < 1024; ++attempt)
			{
				if (transfer.status() != Transfer::Status::Failed ||
					transfer.reason() != Transfer::Reason::ConflictingPage || !transfer.isComplete())
				{
					valid = false;
				}
				std::this_thread::yield();
			}
			++earlyCalls;
		});
		std::jthread completer([&] {
			(void)TransferCompletion::complete(transfer, Transfer::Status::Failed, Transfer::Reason::ConflictingPage);
		});
		callbackEntered.wait();
		std::array<std::jthread, SubscriberCount> subscribers;
		for (std::size_t index = 0; index < SubscriberCount; ++index)
		{
			subscribers[index] = std::jthread([&, index] {
				subscribersStarting.count_down();
				auto contract = transfer.subscribeToCompletion([&, index](Transfer::Status status, Transfer::Reason reason) {
					if (status != transfer.status() || reason != transfer.reason() || !transfer.isComplete())
					{
						valid = false;
					}
					++calls[index];
				});
			});
		}
		completer.join();
		for (std::jthread &subscriber : subscribers)
		{
			subscriber.join();
		}
		for (const auto &count : calls)
		{
			if (count.load() != 1)
			{
				return false;
			}
		}
		return valid.load() && earlyCalls.load() == 1;
	}
}

TEST(TransferDeathTest, ConcurrentSubscribersCannotBlockCompletionQueries)
{
	// A child-process deadline turns a locking regression into a failure instead
	// of leaving the complete test runner blocked in thread destruction.
	GTEST_FLAG_SET(death_test_style, "threadsafe");
	EXPECT_EXIT(std::_Exit(exerciseConcurrentCompletionQueries() ? EXIT_SUCCESS : EXIT_FAILURE), ::testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(Transfer, CopiesObserveOneSharedTerminalOutcome)
{
	Transfer transfer;
	Transfer retained = transfer;
	EXPECT_EQ(transfer.id(), Transfer::InvalidID);
	EXPECT_EQ(retained.status(), Transfer::Status::Pending);
	TransferCompletion::assignID(transfer, 42);
	TransferCompletion::assignID(retained, 43);
	EXPECT_EQ(retained.id(), 42u);
	EXPECT_TRUE(TransferCompletion::complete(retained, Transfer::Status::Success));
	EXPECT_EQ(transfer.status(), Transfer::Status::Success);
	EXPECT_EQ(transfer.reason(), Transfer::Reason::None);
	EXPECT_TRUE(transfer.isComplete());
	EXPECT_FALSE(TransferCompletion::complete(transfer, Transfer::Status::Failed, Transfer::Reason::DuplicatePage));
}

TEST(Transfer, NotifiesExactlyOnceAndLateSubscribersSeeCompletion)
{
	Transfer transfer;
	int earlier = 0;
	int later = 0;
	auto first = transfer.subscribeToCompletion([&](Transfer::Status status, Transfer::Reason reason) {
		EXPECT_EQ(status, Transfer::Status::Failed);
		EXPECT_EQ(reason, Transfer::Reason::ConflictingPage);
		++earlier;
	});
	EXPECT_TRUE(TransferCompletion::complete(transfer, Transfer::Status::Failed, Transfer::Reason::ConflictingPage));
	EXPECT_FALSE(TransferCompletion::complete(transfer, Transfer::Status::Indeterminate));
	auto second = transfer.subscribeToCompletion([&](Transfer::Status status, Transfer::Reason reason) {
		EXPECT_EQ(status, Transfer::Status::Failed);
		EXPECT_EQ(reason, Transfer::Reason::ConflictingPage);
		++later;
	});
	EXPECT_EQ(earlier, 1);
	EXPECT_EQ(later, 1);
	EXPECT_TRUE(first.isValid());
	EXPECT_TRUE(second.isValid());
}

TEST(Transfer, CompletionAndSubscriptionRaceStillDeliversExactlyOnce)
{
	for (int iteration = 0; iteration < 100; ++iteration)
	{
		Transfer transfer;
		std::atomic_int calls = 0;
		std::thread worker([&] {
			(void)TransferCompletion::complete(transfer, Transfer::Status::Rejected, Transfer::Reason::ReceiverCapacity);
		});
		auto subscription = transfer.subscribeToCompletion([&](Transfer::Status, Transfer::Reason) {
			++calls;
		});
		worker.join();
		EXPECT_EQ(calls.load(), 1);
	}
}

TEST(Transfer, DroppingApplicationCopiesDoesNotCompleteTransfer)
{
	Transfer transport;
	{
		Transfer user = transport;
		EXPECT_FALSE(user.isComplete());
	}
	EXPECT_EQ(transport.status(), Transfer::Status::Pending);
	EXPECT_TRUE(TransferCompletion::complete(transport, Transfer::Status::Indeterminate, Transfer::Reason::AcknowledgmentTimeout));
	EXPECT_EQ(transport.reason(), Transfer::Reason::AcknowledgmentTimeout);
}

TEST(Transfer, CompletionCallbackCanSubscribeAndQuerySameHandle)
{
	Transfer transfer;
	Transfer::CompletionContract nested;
	std::uint32_t earlier = 0;
	std::uint32_t later = 0;
	auto original = transfer.subscribeToCompletion([&](Transfer::Status status, Transfer::Reason reason) {
		EXPECT_EQ(transfer.status(), status);
		EXPECT_EQ(transfer.reason(), reason);
		EXPECT_TRUE(transfer.isComplete());
		++earlier;
		nested = transfer.subscribeToCompletion([&](Transfer::Status nestedStatus, Transfer::Reason nestedReason) {
			EXPECT_EQ(nestedStatus, Transfer::Status::Failed);
			EXPECT_EQ(nestedReason, Transfer::Reason::ConflictingPage);
			EXPECT_EQ(transfer.status(), nestedStatus);
			EXPECT_EQ(transfer.reason(), nestedReason);
			++later;
		});
	});
	EXPECT_TRUE(TransferCompletion::complete(transfer, Transfer::Status::Failed, Transfer::Reason::ConflictingPage));
	EXPECT_FALSE(TransferCompletion::complete(transfer, Transfer::Status::Success));
	EXPECT_EQ(earlier, 1u);
	EXPECT_EQ(later, 1u);
	EXPECT_TRUE(original.isValid());
	EXPECT_TRUE(nested.isValid());
}
