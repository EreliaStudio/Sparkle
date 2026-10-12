#include "network/internal/page_scheduler.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace
{
	[[nodiscard]] spk::Message message(std::size_t size, std::uint32_t type = 1)
	{
		spk::Message::Writer writer(type);
		writer.resize(size);
		for (std::size_t index = 0; index < size; ++index)
		{
			writer.data()[index] = static_cast<std::byte>(index % 251);
		}
		return std::move(writer).build();
	}
}

TEST(PageScheduler, EmptySchedulerProducesNoPages)
{
	spk::NetworkInternal::PageScheduler scheduler;
	EXPECT_FALSE(scheduler.nextPage(128).has_value());
	EXPECT_EQ(scheduler.pendingTransfers(), 0u);
}

TEST(PageScheduler, LargeTransferUsesSmallPages)
{
	spk::NetworkInternal::PageScheduler scheduler;
	const auto transferID = scheduler.enqueue(message(10));
	std::vector<std::size_t> offsets;
	while (const auto page = scheduler.nextPage(3))
	{
		EXPECT_EQ(page->transferID, transferID);
		EXPECT_EQ(page->totalSize, 10u);
		EXPECT_LE(page->length, 3u);
		offsets.push_back(page->offset);
	}
	EXPECT_EQ(offsets, (std::vector<std::size_t>{0, 3, 6, 9}));
	EXPECT_EQ(scheduler.pendingBytes(), 0u);
}

TEST(PageScheduler, EqualPriorityTransfersUseRoundRobin)
{
	spk::NetworkInternal::PageScheduler scheduler;
	const auto first = scheduler.enqueue(message(6, 1));
	const auto second = scheduler.enqueue(message(6, 2));
	std::vector<std::uint64_t> order;
	while (const auto page = scheduler.nextPage(2))
	{
		order.push_back(page->transferID);
	}
	EXPECT_EQ(order, (std::vector<std::uint64_t>{first, second, first, second, first, second}));
}

TEST(PageScheduler, HigherPriorityOvertakesExistingLowPriorityTransfer)
{
	spk::NetworkInternal::PageScheduler scheduler;
	const auto low = scheduler.enqueue(message(100), spk::Message::Priority::Low);
	ASSERT_EQ(scheduler.nextPage(1)->transferID, low);
	const auto high = scheduler.enqueue(message(2), spk::Message::Priority::High);
	bool overtook = false;
	for (int index = 0; index < 10; ++index)
	{
		const auto page = scheduler.nextPage(1);
		ASSERT_TRUE(page.has_value());
		if (page->transferID == high)
		{
			overtook = true;
			break;
		}
	}
	EXPECT_TRUE(overtook);
}

TEST(PageScheduler, LowPriorityEventuallyGetsTimeUnderHighPriorityLoad)
{
	spk::NetworkInternal::PageScheduler scheduler;
	const auto high = scheduler.enqueue(message(1000), spk::Message::Priority::High);
	const auto low = scheduler.enqueue(message(10), spk::Message::Priority::Low);
	std::size_t lowPages = 0;
	for (int index = 0; index < 70; ++index)
	{
		const auto page = scheduler.nextPage(1);
		ASSERT_TRUE(page.has_value());
		EXPECT_TRUE(page->transferID == low || page->transferID == high);
		lowPages += page->transferID == low ? 1u : 0u;
	}
	EXPECT_EQ(lowPages, 10u);
}

TEST(PageScheduler, PendingBudgetUsesSaturationThresholdNotHardAllocationCap)
{
	spk::NetworkInternal::PageScheduler scheduler(1);
	const auto first = scheduler.enqueue(message(20));
	EXPECT_EQ(scheduler.pendingBytes(), 20u);
	EXPECT_THROW((void)scheduler.enqueue(message(1)), spk::Exception);
	EXPECT_EQ(scheduler.nextPage(30)->transferID, first);
	EXPECT_EQ(scheduler.pendingBytes(), 0u);
	EXPECT_NO_THROW((void)scheduler.enqueue(message(1)));
}

TEST(PageScheduler, ReducingBudgetPreservesExistingTransfers)
{
	spk::NetworkInternal::PageScheduler scheduler(128);
	const auto id = scheduler.enqueue(message(50));
	scheduler.setMaxPendingBytes(1);
	EXPECT_EQ(scheduler.pendingBytes(), 50u);
	EXPECT_THROW((void)scheduler.enqueue(message(5)), spk::Exception);
	EXPECT_EQ(scheduler.nextPage(50)->transferID, id);
}

TEST(PageScheduler, EmptyMessageStillEmitsOnePage)
{
	spk::NetworkInternal::PageScheduler scheduler;
	const auto id = scheduler.enqueue(message(0));
	const auto page = scheduler.nextPage(32);
	ASSERT_TRUE(page.has_value());
	EXPECT_EQ(page->transferID, id);
	EXPECT_EQ(page->length, 0u);
	EXPECT_FALSE(scheduler.nextPage(32).has_value());
}

TEST(PageScheduler, ValidatesPriorityAndPageSize)
{
	spk::NetworkInternal::PageScheduler scheduler;
	EXPECT_THROW((void)scheduler.enqueue(message(1), static_cast<spk::Message::Priority>(99)), spk::Exception);
	EXPECT_THROW((void)scheduler.nextPage(0), spk::Exception);
	EXPECT_THROW((void)scheduler.nextPage(spk::NetworkInternal::MaximumPayloadSize), spk::Exception);
}

TEST(PageScheduler, TrackedTransferCompletesOnlyAfterAcknowledgment)
{
	spk::NetworkInternal::PageScheduler scheduler;
	auto transfer = scheduler.enqueueTracked(message(10));
	EXPECT_NE(transfer.id(), spk::Transfer::InvalidID);
	while (scheduler.nextPage(3))
	{
	}
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Pending);
	EXPECT_TRUE(scheduler.complete(transfer.id(), spk::Transfer::Status::Success));
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Success);
	EXPECT_FALSE(scheduler.complete(transfer.id(), spk::Transfer::Status::Failed));
}

TEST(PageScheduler, FailureDiscardsRemainingPagesWithoutAffectingOthers)
{
	spk::NetworkInternal::PageScheduler scheduler;
	auto failed = scheduler.enqueueTracked(message(40));
	auto healthy = scheduler.enqueueTracked(message(12));
	ASSERT_TRUE(scheduler.nextPage(4).has_value());
	EXPECT_TRUE(scheduler.complete(failed.id(), spk::Transfer::Status::Rejected, spk::Transfer::Reason::ReceiverCapacity));
	EXPECT_EQ(failed.reason(), spk::Transfer::Reason::ReceiverCapacity);
	while (const auto page = scheduler.nextPage(4))
	{
		EXPECT_EQ(page->transferID, healthy.id());
	}
	EXPECT_EQ(healthy.status(), spk::Transfer::Status::Pending);
	EXPECT_TRUE(scheduler.complete(healthy.id(), spk::Transfer::Status::Success));
}

TEST(PageScheduler, ClearLeavesRetainedTransfersIndeterminate)
{
	spk::NetworkInternal::PageScheduler scheduler;
	auto transfer = scheduler.enqueueTracked(message(100));
	int notified = 0;
	auto contract = transfer.subscribeToCompletion([&](spk::Transfer::Status status, spk::Transfer::Reason reason) {
		EXPECT_EQ(status, spk::Transfer::Status::Indeterminate);
		EXPECT_EQ(reason, spk::Transfer::Reason::TransportFailure);
		++notified;
	});
	scheduler.clear();
	EXPECT_EQ(transfer.status(), spk::Transfer::Status::Indeterminate);
	EXPECT_EQ(scheduler.pendingBytes(), 0u);
	EXPECT_EQ(notified, 1);
	scheduler.clear();
	EXPECT_EQ(notified, 1);
}
