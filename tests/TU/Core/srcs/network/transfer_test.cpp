#include "network/internal/transfer_completion.hpp"
#include "network/transfer.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

namespace
{
	using spk::Transfer;
	using spk::NetworkInternal::TransferCompletion;
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
