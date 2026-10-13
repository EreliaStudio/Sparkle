#include "network/internal/server_send_batch.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory>

#include <utility>
#include <vector>

TEST(ServerSendBatch, AtomicRejectionPreservesCausesAndIdentifiesCancelledRecipients)
{
	using Batch = spk::NetworkInternal::ServerSendBatch;
	using Reason = spk::Transfer::Reason;
	spk::Message message = spk::Message::Writer(1).build();
	Batch batch(message, spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::Reject);
	batch.recipients.push_back({spk::Connection{}, spk::Transfer{}});
	batch.recipients.push_back({spk::Connection{}, spk::Transfer{}});
	batch.recipients.push_back({spk::Connection{}, spk::Transfer{}});
	auto first = batch.recipients[0].transfer;
	auto second = batch.recipients[1].transfer;
	auto third = batch.recipients[2].transfer;
	batch.rejectAll({{&batch.recipients[1], Reason::QueueOverflow}, {&batch.recipients[2], Reason::InvalidRecipient}});
	EXPECT_EQ(first.status(), spk::Transfer::Status::Rejected);
	EXPECT_EQ(first.reason(), Reason::AtomicBatchRejected);
	EXPECT_EQ(second.status(), spk::Transfer::Status::Rejected);
	EXPECT_EQ(second.reason(), Reason::QueueOverflow);
	EXPECT_EQ(third.status(), spk::Transfer::Status::Rejected);
	EXPECT_EQ(third.reason(), Reason::InvalidRecipient);
}

TEST(ServerSendBatch, ExplicitShutdownCancellationUnblocksAWaiterWhileCallerOwnsBatch)
{
	using Batch = spk::NetworkInternal::ServerSendBatch;
	const auto message = spk::Message::Writer(10).build();
	auto batch = std::make_shared<Batch>(
		message, spk::Message::Priority::Normal, spk::Connection::OverflowPolicy::Reject);
	auto admission = batch->admission.get_future();
	auto retained = batch;
	batch->cancelAdmission();
	EXPECT_EQ(admission.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
	EXPECT_THROW(admission.get(), spk::Exception);
	EXPECT_EQ(batch, retained);
	batch->cancelAdmission();
}
