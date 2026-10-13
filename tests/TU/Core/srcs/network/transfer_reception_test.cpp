#include "network/internal/page_scheduler.hpp"
#include "network/internal/transfer_reception.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <utility>

namespace
{
	spk::Message message(std::size_t length)
	{
		spk::Message::Writer writer(14);
		writer.resize(length);
		return std::move(writer).build();
	}

	spk::NetworkInternal::PageFragment fragment(const spk::NetworkInternal::Page &page)
	{
		return {page.message.header(), page.transferID, page.totalSize, page.offset, page.data()};
	}
}

TEST(TransferReception, OnlyCompletedMessagesProduceSuccess)
{
	spk::NetworkInternal::PageScheduler scheduler;
	spk::NetworkInternal::TransferReception reception;
	(void)scheduler.enqueue(message(8));
	const auto first = scheduler.nextPage(4);
	const auto last = scheduler.nextPage(4);
	auto pending = reception.accept(fragment(*first), 56);
	EXPECT_FALSE(pending.message.has_value());
	EXPECT_FALSE(pending.response.has_value());
	auto result = reception.accept(fragment(*last), 56);
	ASSERT_TRUE(result.message.has_value());
	ASSERT_TRUE(result.response.has_value());
	EXPECT_EQ(result.response->transferID, first->transferID);
	EXPECT_EQ(result.response->status, spk::Transfer::Status::Success);
	EXPECT_EQ(result.message->size(), 8u);
}

TEST(TransferReception, InvalidPageFailsOneTransferNotSession)
{
	spk::NetworkInternal::PageScheduler scheduler;
	spk::NetworkInternal::TransferReception reception;
	(void)scheduler.enqueue(message(8));
	const auto first = scheduler.nextPage(4);
	ASSERT_FALSE(reception.accept(fragment(*first), 56).message.has_value());
	auto conflict = fragment(*first);
	const auto failure = reception.accept(conflict, 56);
	ASSERT_TRUE(failure.response.has_value());
	EXPECT_EQ(failure.response->status, spk::Transfer::Status::Failed);
	EXPECT_EQ(failure.response->reason, spk::Transfer::Reason::DuplicatePage);
	EXPECT_FALSE(reception.failureLimitReached());
	EXPECT_EQ(reception.reservedBytes(), 0u);
}

TEST(TransferReception, TimeoutProducesTargetedFailure)
{
	spk::NetworkInternal::PageScheduler scheduler;
	spk::NetworkInternal::TransferReception reception;
	(void)scheduler.enqueue(message(8));
	const auto first = scheduler.nextPage(4);
	const auto now = spk::NetworkInternal::TransferReception::Clock::now();
	ASSERT_FALSE(reception.accept(fragment(*first), 56, now).message.has_value());
	const auto expired = reception.expire(56, now + std::chrono::seconds(60));
	ASSERT_EQ(expired.size(), 1u);
	EXPECT_EQ(expired.front().transferID, first->transferID);
	EXPECT_EQ(expired.front().reason, spk::Transfer::Reason::InactivityTimeout);
	EXPECT_TRUE(reception.expire(56, now + std::chrono::seconds(61)).empty());
}

TEST(TransferReception, ConfigurableTimeoutProducesFailureForExpiredTransfer)
{
	spk::NetworkInternal::PageScheduler scheduler;
	spk::NetworkInternal::TransferReception reception;
	reception.setInactivityTimeout(std::chrono::milliseconds(100));
	(void)scheduler.enqueue(message(12));
	const auto first = scheduler.nextPage(4);
	const auto second = scheduler.nextPage(4);
	const auto now = spk::NetworkInternal::TransferReception::Clock::now();
	ASSERT_FALSE(reception.accept(fragment(*first), 56, now).message.has_value());
	ASSERT_FALSE(reception.accept(fragment(*second), 56, now + std::chrono::milliseconds(90)).message.has_value());
	EXPECT_TRUE(reception.expire(56, now + std::chrono::milliseconds(150)).empty());
	const auto expired = reception.expire(56, now + std::chrono::milliseconds(190));
	ASSERT_EQ(expired.size(), 1u);
	EXPECT_EQ(expired.front().reason, spk::Transfer::Reason::InactivityTimeout);
	EXPECT_EQ(reception.reservedBytes(), 0u);
}
