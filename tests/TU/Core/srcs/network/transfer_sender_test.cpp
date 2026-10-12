#include "network/internal/transfer_sender.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <utility>

namespace
{
	spk::Message makeMessage(std::size_t size)
	{
		spk::Message::Writer writer(22);
		writer.resize(size);
		return std::move(writer).build();
	}
}

TEST(TransferSender, FullPageGenerationDoesNotStartAcknowledgmentTimeout)
{
	spk::NetworkInternal::TransferSender sender;
	const auto now = spk::NetworkInternal::AcknowledgmentDeadlines::Clock::now();
	auto transfer = sender.enqueue(makeMessage(8));
	while (sender.nextPage(4))
	{
	}
	sender.expire(now + std::chrono::minutes(2));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Pending);
	sender.finalPageWritten(transfer.id(), now);
	sender.expire(now + std::chrono::seconds(30));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Indeterminate);
	EXPECT_EQ(transfer.reason(), spk::Transfer::Reason::AcknowledgmentTimeout);
}

TEST(TransferSender, PositiveAcknowledgmentCompletesWithoutTimeout)
{
	spk::NetworkInternal::TransferSender sender;
	const auto now = spk::NetworkInternal::AcknowledgmentDeadlines::Clock::now();
	auto transfer = sender.enqueue(makeMessage(12));
	sender.finalPageWritten(transfer.id(), now);
	const auto response = spk::NetworkInternal::TransferResponse{11, transfer.id(), spk::Transfer::Status::Success, spk::Transfer::Reason::None};
	EXPECT_TRUE(sender.acknowledge(response));
	sender.expire(now + std::chrono::minutes(1));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
	EXPECT_FALSE(sender.acknowledge(response));
}

TEST(TransferSender, RejectionCancelsQueuedPagesWithoutDisconnect)
{
	spk::NetworkInternal::TransferSender sender;
	auto transfer = sender.enqueue(makeMessage(12));
	auto other = sender.enqueue(makeMessage(16));
	const auto response = spk::NetworkInternal::TransferResponse{11, transfer.id(), spk::Transfer::Status::Rejected, spk::Transfer::Reason::ReceiverCapacity};
	EXPECT_TRUE(sender.acknowledge(response));
	while (const auto page = sender.nextPage(4))
	{
		EXPECT_EQ(page->transferID, other.id());
	}
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Rejected);
	EXPECT_EQ(other.status(), spk::Transfer::Status::Pending);
	sender.clear();
	EXPECT_EQ(other.status(), spk::Transfer::Status::Indeterminate);
}
