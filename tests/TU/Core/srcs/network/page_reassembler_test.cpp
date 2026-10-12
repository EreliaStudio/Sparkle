#include "network/internal/page_reassembler.hpp"
#include "network/internal/page_scheduler.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace
{
	[[nodiscard]] spk::Message message(std::size_t size, std::uint32_t type)
	{
		spk::Message::Writer writer(type);
		writer.setRequestID(123);
		writer.setConnectionID(456);
		writer.resize(size);
		for (std::size_t index = 0; index < size; ++index)
		{
			writer.data()[index] = static_cast<std::byte>((index * 7) % 251);
		}
		return std::move(writer).build();
	}

	[[nodiscard]] spk::NetworkInternal::PageFragment fragment(const spk::NetworkInternal::Page &page)
	{
		return {page.message.header(), page.transferID, page.totalSize, page.offset, page.data()};
	}
}

TEST(PageReassembler, ReconstructsLargeMultiPagePayloadAndMetadata)
{
	auto original = message(128 * 1024, 28);
	spk::NetworkInternal::PageScheduler scheduler;
	spk::NetworkInternal::PageReassembler assembler;
	(void)scheduler.enqueue(original);
	std::optional<spk::Message> completed;
	std::size_t pages = 0;
	while (const auto page = scheduler.nextPage(2000))
	{
		const auto encoded = spk::NetworkInternal::encodePage(*page);
		const auto received = spk::NetworkInternal::decodePage(*encoded);
		completed = assembler.accept(received);
		++pages;
		if (scheduler.pendingTransfers() != 0)
		{
			EXPECT_FALSE(completed.has_value());
		}
	}
	ASSERT_GT(pages, 2u);
	ASSERT_TRUE(completed.has_value());
	EXPECT_EQ(completed->type(), original.type());
	EXPECT_EQ(completed->requestID(), original.requestID());
	EXPECT_EQ(completed->connectionID(), original.connectionID());
	EXPECT_TRUE(std::equal(completed->data().begin(), completed->data().end(), original.data().begin()));
	EXPECT_EQ(assembler.reservedBytes(), 0u);
}

TEST(PageReassembler, AcceptsInterleavedTransfersWithoutDispatchingPartialData)
{
	spk::NetworkInternal::PageScheduler scheduler;
	spk::NetworkInternal::PageReassembler assembler;
	(void)scheduler.enqueue(message(17, 1));
	(void)scheduler.enqueue(message(19, 2));
	std::vector<std::uint32_t> completed;
	while (const auto page = scheduler.nextPage(3))
	{
		auto result = assembler.accept(fragment(*page));
		if (result.has_value())
		{
			completed.push_back(result->type());
		}
	}
	std::sort(completed.begin(), completed.end());
	EXPECT_EQ(completed, (std::vector<std::uint32_t>{1, 2}));
}

TEST(PageReassembler, RejectsFirstPageOverLogicalAndAggregateBudgets)
{
	spk::NetworkInternal::PageScheduler scheduler;
	auto page = scheduler.enqueue(message(11, 7));
	(void)page;
	const auto outgoing = scheduler.nextPage(3);
	spk::NetworkInternal::PageReassembler tooLarge(10, 100);
	EXPECT_THROW((void)tooLarge.accept(fragment(*outgoing)), spk::Exception);
	spk::NetworkInternal::PageReassembler tooMuchMemory(100, 10);
	EXPECT_THROW((void)tooMuchMemory.accept(fragment(*outgoing)), spk::Exception);
	EXPECT_EQ(tooLarge.reservedBytes(), 0u);
}

TEST(PageReassembler, RejectsUnknownTransferStartingAwayFromOffsetZero)
{
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(15, 1));
	(void)scheduler.nextPage(5);
	auto latePage = scheduler.nextPage(5);
	spk::NetworkInternal::PageReassembler assembler;
	EXPECT_THROW((void)assembler.accept(fragment(*latePage)), spk::Exception);
}

TEST(PageReassembler, RejectsConflictingDuplicateAndOverlappingPages)
{
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(20, 5));
	auto first = scheduler.nextPage(5);
	spk::NetworkInternal::PageReassembler assembler;
	const auto received = fragment(*first);
	EXPECT_FALSE(assembler.accept(received).has_value());
	EXPECT_THROW((void)assembler.accept(received), spk::NetworkInternal::PageReassemblyError);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
	EXPECT_FALSE(assembler.accept(received).has_value());

	spk::NetworkInternal::PageReassembler conflictingAssembler;
	EXPECT_FALSE(conflictingAssembler.accept(received).has_value());
	std::vector<std::byte> corrupted(received.data.begin(), received.data.end());
	corrupted[0] = std::byte{0xFF};
	auto conflicting = received;
	conflicting.data = corrupted;
	try
	{
		(void)conflictingAssembler.accept(conflicting);
		FAIL() << "Expected conflicting page failure.";
	} catch (const spk::NetworkInternal::PageReassemblyError &error)
	{
		EXPECT_EQ(error.reason(), spk::NetworkInternal::PageReassemblyError::Reason::ConflictingPage);
	}

	spk::NetworkInternal::PageReassembler overlapAssembler;
	EXPECT_FALSE(overlapAssembler.accept(received).has_value());
	auto overlapping = received;
	overlapping.offset = 2;
	EXPECT_THROW((void)overlapAssembler.accept(overlapping), spk::NetworkInternal::PageReassemblyError);
	EXPECT_EQ(overlapAssembler.reservedBytes(), 0u);
}

TEST(PageReassembler, RejectsMismatchedMetadataAndMalformedFrameOffsets)
{
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(20, 5));
	const auto first = scheduler.nextPage(5);
	spk::NetworkInternal::PageReassembler assembler;
	EXPECT_FALSE(assembler.accept(fragment(*first)).has_value());
	auto badMetadata = fragment(*first);
	badMetadata.header.requestID = 999;
	EXPECT_THROW((void)assembler.accept(badMetadata), spk::Exception);

	auto frame = spk::NetworkInternal::encodePage(*first);
	spk::NetworkInternal::write64(frame->data() + spk::NetworkInternal::FrameHeaderSize + 16, 50);
	const auto invalidOffset = spk::NetworkInternal::decodePage(*frame);
	spk::NetworkInternal::PageReassembler offsetAssembler;
	EXPECT_THROW((void)offsetAssembler.accept(invalidOffset), spk::NetworkInternal::PageReassemblyError);
}

TEST(PageReassembler, TimedOutTransfersReleaseTheirReservedMemory)
{
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(200, 9));
	auto first = scheduler.nextPage(3);
	spk::NetworkInternal::PageReassembler assembler(1000, 1000, std::chrono::milliseconds(100));
	const auto now = spk::NetworkInternal::PageReassembler::Clock::now();
	EXPECT_FALSE(assembler.accept(fragment(*first), now).has_value());
	EXPECT_EQ(assembler.reservedBytes(), 200u);
	EXPECT_TRUE(assembler.expire(now + std::chrono::milliseconds(99)).empty());
	EXPECT_EQ(assembler.pendingTransfers(), 1u);
	const auto expired = assembler.expire(now + std::chrono::milliseconds(101));
	ASSERT_EQ(expired.size(), 1u);
	EXPECT_EQ(expired.front(), first->transferID);
	EXPECT_TRUE(assembler.expire(now + std::chrono::milliseconds(102)).empty());
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
}

TEST(PageReassembler, HandlesEmptyAndExactBoundaryPayloads)
{
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(0, 1));
	(void)scheduler.enqueue(message(8, 2));
	spk::NetworkInternal::PageReassembler assembler;
	std::size_t completed = 0;
	while (const auto page = scheduler.nextPage(4))
	{
		const auto encoded = spk::NetworkInternal::encodePage(*page);
		const auto fragment = spk::NetworkInternal::decodePage(*encoded);
		completed += assembler.accept(fragment).has_value() ? 1u : 0u;
	}
	EXPECT_EQ(completed, 2u);
}

TEST(PageReassembler, ValidPagesRefreshTheIncomingInactivityDeadline)
{
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(12, 8));
	const auto first = scheduler.nextPage(4);
	const auto second = scheduler.nextPage(4);
	spk::NetworkInternal::PageReassembler assembler(100, 100, std::chrono::milliseconds(100));
	const auto now = spk::NetworkInternal::PageReassembler::Clock::now();
	EXPECT_FALSE(assembler.accept(fragment(*first), now).has_value());
	EXPECT_FALSE(assembler.accept(fragment(*second), now + std::chrono::milliseconds(90)).has_value());
	assembler.expire(now + std::chrono::milliseconds(170));
	EXPECT_EQ(assembler.pendingTransfers(), 1u);
	assembler.expire(now + std::chrono::milliseconds(191));
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
}

TEST(PageReassembler, TenConsecutiveMalformedTransfersReachAbuseThreshold)
{
	spk::NetworkInternal::PageScheduler scheduler;
	spk::NetworkInternal::PageReassembler assembler;
	for (std::size_t index = 0; index < 10; ++index)
	{
		(void)scheduler.enqueue(message(10, static_cast<std::uint32_t>(index)));
	}
	for (std::size_t index = 0; index < 10; ++index)
	{
		const auto first = scheduler.nextPage(3);
		auto malformed = fragment(*first);
		malformed.offset = 1;
		EXPECT_THROW((void)assembler.accept(malformed), spk::NetworkInternal::PageReassemblyError);
		EXPECT_EQ(assembler.failureLimitReached(), index == 9);
	}
}

TEST(PageReassembler, ExpiredTransferRejectsLatePagesWithoutReallocation)
{
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(20, 17));
	const auto first = scheduler.nextPage(5);
	const auto later = scheduler.nextPage(5);
	spk::NetworkInternal::PageReassembler assembler(100, 100, std::chrono::milliseconds(50));
	const auto now = spk::NetworkInternal::PageReassembler::Clock::now();
	ASSERT_FALSE(assembler.accept(fragment(*first), now).has_value());
	EXPECT_EQ(assembler.expire(now + std::chrono::milliseconds(50)).size(), 1u);
	EXPECT_FALSE(assembler.accept(fragment(*later), now + std::chrono::milliseconds(51)).has_value());
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
}
