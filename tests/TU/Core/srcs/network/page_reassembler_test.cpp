#include "network/internal/page_reassembler.hpp"
#include "network/internal/page_scheduler.hpp"
#include "network/internal/reassembly_budget.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <latch>
#include <limits>
#include <memory>
#include <thread>
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
	auto budget = std::make_shared<spk::NetworkInternal::ReassemblyBudget>(4096);
	spk::NetworkInternal::PageReassembler assembler(100, 4096, std::chrono::milliseconds(100), budget);
	const auto now = spk::NetworkInternal::PageReassembler::Clock::now();
	EXPECT_FALSE(assembler.accept(fragment(*first), now).has_value());
	EXPECT_GT(budget->used(), 12u);
	EXPECT_FALSE(assembler.accept(fragment(*second), now + std::chrono::milliseconds(90)).has_value());
	assembler.expire(now + std::chrono::milliseconds(170));
	EXPECT_EQ(assembler.pendingTransfers(), 1u);
	assembler.expire(now + std::chrono::milliseconds(191));
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
	EXPECT_EQ(budget->used(), 0u);
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
	auto budget = std::make_shared<spk::NetworkInternal::ReassemblyBudget>(4096);
	spk::NetworkInternal::PageReassembler assembler(100, 4096, std::chrono::milliseconds(50), budget);
	const auto now = spk::NetworkInternal::PageReassembler::Clock::now();
	ASSERT_FALSE(assembler.accept(fragment(*first), now).has_value());
	EXPECT_GT(budget->used(), 20u);
	EXPECT_EQ(assembler.expire(now + std::chrono::milliseconds(50)).size(), 1u);
	EXPECT_FALSE(assembler.accept(fragment(*later), now + std::chrono::milliseconds(51)).has_value());
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
	EXPECT_EQ(budget->used(), 0u);
}

TEST(PageReassembler, RuntimeInactivityTimeoutChangesExistingTransferDeadline)
{
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(20, 61));
	const auto first = scheduler.nextPage(5);
	spk::NetworkInternal::PageReassembler assembler;
	const auto now = spk::NetworkInternal::PageReassembler::Clock::now();
	ASSERT_FALSE(assembler.accept(fragment(*first), now).has_value());
	assembler.setInactivityTimeout(std::chrono::milliseconds(100));
	EXPECT_TRUE(assembler.expire(now + std::chrono::milliseconds(99)).empty());
	EXPECT_EQ(assembler.expire(now + std::chrono::milliseconds(100)).size(), 1u);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
	EXPECT_THROW(assembler.setInactivityTimeout(std::chrono::milliseconds(0)), spk::Exception);
	EXPECT_THROW(assembler.setInactivityTimeout(std::chrono::milliseconds(-1)), spk::Exception);
}

TEST(PageReassembler, SharedBudgetRejectsOnlyNewTransferAndReclaimsOnClear)
{
	auto budget = std::make_shared<spk::NetworkInternal::ReassemblyBudget>(4096);
	spk::NetworkInternal::PageReassembler first(1024, 4096, std::chrono::milliseconds(100), budget);
	spk::NetworkInternal::PageReassembler second(1024, 4096, std::chrono::milliseconds(100), budget);
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(128, 90));
	const auto initial = scheduler.nextPage(16);
	ASSERT_TRUE(initial.has_value());
	const auto now = spk::NetworkInternal::PageReassembler::Clock::now();
	EXPECT_FALSE(first.accept(fragment(*initial), now).has_value());
	const auto charged = budget->used();
	EXPECT_GT(charged, 128u);
	budget->setMaximum(charged * 2 - 1);
	try
	{
		(void)second.accept(fragment(*initial), now);
		FAIL() << "Expected the shared reassembly budget to reject the new transfer.";
	} catch (const spk::NetworkInternal::PageReassemblyError &error)
	{
		EXPECT_EQ(error.reason(), spk::NetworkInternal::PageReassemblyError::Reason::Capacity);
	}
	EXPECT_EQ(budget->used(), charged);
	first.clear();
	EXPECT_EQ(budget->used(), 0u);
	(void)second.expire(now + std::chrono::milliseconds(100));
	EXPECT_FALSE(second.accept(fragment(*initial), now + std::chrono::milliseconds(101)).has_value());
	EXPECT_EQ(budget->used(), 0u);
	auto fresh = fragment(*initial);
	++fresh.transferID;
	EXPECT_FALSE(second.accept(fresh, now + std::chrono::milliseconds(101)).has_value());
	EXPECT_EQ(budget->used(), charged);
	second.clear();
	EXPECT_EQ(budget->used(), 0u);
}

TEST(PageReassembler, SharedBudgetIsReleasedOnNormalCompletionAndExpiration)
{
	auto budget = std::make_shared<spk::NetworkInternal::ReassemblyBudget>(4096);
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(24, 91));
	const auto firstPage = scheduler.nextPage(12);
	const auto secondPage = scheduler.nextPage(12);
	spk::NetworkInternal::PageReassembler assembler(1024, 4096, std::chrono::milliseconds(50), budget);
	const auto now = spk::NetworkInternal::PageReassembler::Clock::now();
	EXPECT_FALSE(assembler.accept(fragment(*firstPage), now).has_value());
	EXPECT_GT(budget->used(), 24u);
	EXPECT_TRUE(assembler.accept(fragment(*secondPage), now).has_value());
	EXPECT_EQ(budget->used(), 0u);
	(void)scheduler.enqueue(message(24, 92));
	const auto next = scheduler.nextPage(12);
	EXPECT_FALSE(assembler.accept(fragment(*next), now).has_value());
	EXPECT_GT(budget->used(), 0u);
	EXPECT_EQ(assembler.expire(now + std::chrono::milliseconds(50)).size(), 1u);
	EXPECT_EQ(budget->used(), 0u);
}

TEST(PageReassembler, RuntimeLimitsRejectInvalidSettingsWithoutChangingActiveReservations)
{
	spk::NetworkInternal::PageReassembler assembler;
	EXPECT_THROW(assembler.setLimits(0, 512), spk::Exception);
	EXPECT_THROW(assembler.setLimits(512, 0), spk::Exception);
	assembler.setLimits(12, 4096);
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(13, 93));
	const auto first = scheduler.nextPage(4);
	EXPECT_THROW((void)assembler.accept(fragment(*first)), spk::NetworkInternal::PageReassemblyError);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
}

TEST(PageReassembler, LowerLocalBudgetPreservesAcceptedTransferAndConstrainsNewAdmissions)
{
	using namespace spk::NetworkInternal;
	auto budget = std::make_shared<ReassemblyBudget>(4096);
	PageReassembler assembler(4096, 4096, std::chrono::seconds(60), budget);
	const auto original = message(128, 111);
	const Page first{original, 1, original.size(), 0, 16};
	ASSERT_FALSE(assembler.accept(fragment(first)).has_value());
	const auto reduced = budget->used();
	assembler.setLimits(4096, reduced);
	auto rejected = fragment(first);
	rejected.transferID = 2;
	EXPECT_THROW((void)assembler.accept(rejected), PageReassemblyError);
	EXPECT_EQ(budget->used(), reduced);
	const auto completed = assembler.accept(fragment(Page{original, 1, original.size(), 16, original.size() - 16}));
	ASSERT_TRUE(completed.has_value());
	ASSERT_EQ(completed->size(), original.size());
	EXPECT_TRUE(std::equal(completed->data().begin(), completed->data().end(), original.data().begin()));
	EXPECT_EQ(budget->used(), 0u);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
	const auto tooLarge = message(reduced + 1, 112);
	EXPECT_THROW((void)assembler.accept(fragment(Page{tooLarge, 3, tooLarge.size(), 0, 1})), PageReassemblyError);
	const auto small = message(1, 113);
	EXPECT_TRUE(assembler.accept(fragment(Page{small, 4, small.size(), 0, small.size()})).has_value());
	EXPECT_EQ(budget->used(), 0u);
}

TEST(PageReassembler, LowerSharedBudgetPreservesAcceptedTransferAndConstrainsOtherSessions)
{
	using namespace spk::NetworkInternal;
	auto budget = std::make_shared<ReassemblyBudget>(4096);
	PageReassembler first(4096, 4096, std::chrono::seconds(60), budget);
	PageReassembler second(4096, 4096, std::chrono::seconds(60), budget);
	const auto original = message(128, 114);
	const Page initial{original, 1, original.size(), 0, 16};
	ASSERT_FALSE(first.accept(fragment(initial)).has_value());
	const auto reduced = budget->used();
	budget->setMaximum(reduced);
	EXPECT_THROW((void)second.accept(fragment(initial)), PageReassemblyError);
	EXPECT_EQ(budget->used(), reduced);
	EXPECT_TRUE(first.accept(fragment(Page{original, 1, original.size(), 16, original.size() - 16})).has_value());
	EXPECT_EQ(budget->used(), 0u);
	const auto tooLarge = message(reduced + 1, 115);
	EXPECT_THROW((void)second.accept(fragment(Page{tooLarge, 2, tooLarge.size(), 0, 1})), PageReassemblyError);
	const auto small = message(1, 116);
	EXPECT_TRUE(second.accept(fragment(Page{small, 3, small.size(), 0, small.size()})).has_value());
	EXPECT_EQ(budget->used(), 0u);
}

TEST(PageReassembler, LowerLimitsStillBoundExistingTransferMetadata)
{
	using namespace spk::NetworkInternal;
	auto budget = std::make_shared<ReassemblyBudget>(4096);
	const auto original = message(128, 117);
	const Page initial{original, 1, original.size(), 0, 16};
	PageReassembler probe(4096, 4096, std::chrono::seconds(60), budget);
	ASSERT_FALSE(probe.accept(fragment(initial)).has_value());
	const auto exactFirstPageCharge = budget->used();
	probe.clear();
	PageReassembler assembler(4096, exactFirstPageCharge, std::chrono::seconds(60), budget);
	ASSERT_FALSE(assembler.accept(fragment(initial)).has_value());
	assembler.setLimits(4096, 1);
	EXPECT_THROW((void)assembler.accept(fragment(Page{original, 1, original.size(), 16, 16})), PageReassemblyError);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
	EXPECT_EQ(budget->used(), 0u);
}

TEST(PageReassembler, RaisedLocalAllowanceSurvivesLaterReduction)
{
	using namespace spk::NetworkInternal;
	auto budget = std::make_shared<ReassemblyBudget>(4096);
	const auto original = message(128, 118);
	const Page initial{original, 1, original.size(), 0, 16};
	PageReassembler probe(4096, 4096, std::chrono::seconds(60), budget);
	ASSERT_FALSE(probe.accept(fragment(initial)).has_value());
	const auto exactFirstPageCharge = budget->used();
	probe.clear();
	PageReassembler assembler(4096, exactFirstPageCharge, std::chrono::seconds(60), budget);
	ASSERT_FALSE(assembler.accept(fragment(initial)).has_value());
	assembler.setLimits(4096, 4096);
	assembler.setLimits(4096, 1);
	EXPECT_TRUE(assembler.accept(fragment(Page{original, 1, original.size(), 16, original.size() - 16})).has_value());
	EXPECT_EQ(budget->used(), 0u);
}

TEST(PageReassembler, LoweredBudgetsReclaimFailedExpiredAndClearedTransfers)
{
	using namespace spk::NetworkInternal;
	auto budget = std::make_shared<ReassemblyBudget>(4096);
	PageReassembler assembler(4096, 4096, std::chrono::milliseconds(100), budget);
	const auto original = message(128, 119);
	const Page initial{original, 1, original.size(), 0, 16};
	const auto now = PageReassembler::Clock::now();
	ASSERT_FALSE(assembler.accept(fragment(initial), now).has_value());
	assembler.setLimits(4096, 1);
	budget->setMaximum(1);
	EXPECT_THROW((void)assembler.accept(fragment(initial), now), PageReassemblyError);
	EXPECT_EQ(budget->used(), 0u);
	assembler.setLimits(4096, 4096);
	budget->setMaximum(4096);
	auto fresh = fragment(initial);
	fresh.transferID = 2;
	ASSERT_FALSE(assembler.accept(fresh, now).has_value());
	assembler.setLimits(4096, 1);
	budget->setMaximum(1);
	EXPECT_EQ(assembler.expire(now + std::chrono::milliseconds(100)).size(), 1u);
	EXPECT_EQ(budget->used(), 0u);
	assembler.setLimits(4096, 4096);
	budget->setMaximum(4096);
	fresh.transferID = 3;
	ASSERT_FALSE(assembler.accept(fresh, now).has_value());
	assembler.setLimits(4096, 1);
	budget->setMaximum(1);
	assembler.clear();
	EXPECT_EQ(budget->used(), 0u);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
}

TEST(PageReassembler, SharedBudgetReductionPreservesConcurrentAcceptedTransfers)
{
	using namespace spk::NetworkInternal;
	constexpr std::size_t Count = 8;
	auto budget = std::make_shared<ReassemblyBudget>(64u * 1024u);
	const auto original = message(128, 120);
	std::latch admitted(Count);
	std::latch lowered(1);
	std::atomic_bool valid = true;
	std::vector<std::jthread> workers;
	for (std::size_t index = 0; index < Count; ++index)
	{
		workers.emplace_back([&] {
			PageReassembler assembler(4096, 4096, std::chrono::seconds(60), budget);
			bool accepted = false;
			try
			{
				accepted = !assembler.accept(fragment(Page{original, 1, original.size(), 0, 16})).has_value();
			} catch (...)
			{
				valid = false;
			}
			admitted.count_down();
			lowered.wait();
			if (!accepted)
			{
				return;
			}
			try
			{
				(void)assembler.accept(fragment(Page{original, 2, original.size(), 0, 16}));
				valid = false;
			} catch (const PageReassemblyError &error)
			{
				if (error.reason() != PageReassemblyError::Reason::Capacity)
				{
					valid = false;
				}
			}
			try
			{
				if (!assembler.accept(fragment(Page{original, 1, original.size(), 16, original.size() - 16})).has_value())
				{
					valid = false;
				}
			} catch (...)
			{
				valid = false;
			}
		});
	}
	admitted.wait();
	budget->setMaximum(1);
	lowered.count_down();
	for (std::jthread &worker : workers)
	{
		worker.join();
	}
	EXPECT_TRUE(valid.load());
	EXPECT_EQ(budget->used(), 0u);
}

TEST(PageReassembler, CompletedTransferPagesNeverRepublishTheSameLogicalMessage)
{
	spk::NetworkInternal::PageScheduler scheduler;
	spk::NetworkInternal::PageReassembler assembler;
	(void)scheduler.enqueue(message(16, 96));
	const auto first = scheduler.nextPage(8);
	const auto last = scheduler.nextPage(8);
	ASSERT_TRUE(first.has_value());
	ASSERT_TRUE(last.has_value());
	EXPECT_FALSE(assembler.accept(fragment(*first)).has_value());
	ASSERT_TRUE(assembler.accept(fragment(*last)).has_value());
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
	EXPECT_FALSE(assembler.accept(fragment(*first)).has_value());
	EXPECT_FALSE(assembler.accept(fragment(*last)).has_value());
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
}

TEST(PageReassembler, SharedBudgetIsAtomicAcrossConcurrentSessions)
{
	using spk::NetworkInternal::PageReassembler;
	using spk::NetworkInternal::PageReassemblyError;
	auto budget = std::make_shared<spk::NetworkInternal::ReassemblyBudget>(1024);
	spk::NetworkInternal::PageScheduler scheduler;
	(void)scheduler.enqueue(message(256, 98));
	const auto first = scheduler.nextPage(64);
	ASSERT_TRUE(first.has_value());
	std::vector<std::unique_ptr<PageReassembler>> sessions;
	std::vector<std::future<bool>> results;
	std::atomic_bool begin = false;
	for (std::size_t index = 0; index < 24; ++index)
	{
		sessions.push_back(std::make_unique<PageReassembler>(512, 4096, std::chrono::seconds(60), budget));
		auto *session = sessions.back().get();
		results.push_back(std::async(std::launch::async, [session, &first, &begin] {
			while (!begin.load())
			{
				std::this_thread::yield();
			}
			try
			{
				return !session->accept(fragment(*first)).has_value();
			} catch (const PageReassemblyError &error)
			{
				if (error.reason() != PageReassemblyError::Reason::Capacity)
				{
					throw;
				}
				return false;
			}
		}));
	}
	begin = true;
	std::size_t accepted = 0;
	for (auto &result : results)
	{
		accepted += result.get() ? 1 : 0;
	}
	EXPECT_GT(accepted, 0u);
	EXPECT_LT(accepted, sessions.size());
	EXPECT_LE(budget->used(), 1024u);
	for (auto &session : sessions)
	{
		session->clear();
	}
	EXPECT_EQ(budget->used(), 0u);
}

TEST(PageReassembler, ClearIsIdempotentAndAllowsNewTransfersWithSharedBudget)
{
	using namespace spk::NetworkInternal;
	auto budget = std::make_shared<ReassemblyBudget>(4096);
	PageReassembler assembler(1024, 4096, std::chrono::seconds(1), budget);
	PageScheduler scheduler;
	(void)scheduler.enqueue(message(128, 81));
	const auto first = scheduler.nextPage(16);
	ASSERT_TRUE(first.has_value());
	EXPECT_FALSE(assembler.accept(fragment(*first)).has_value());
	EXPECT_GT(budget->used(), 128u);
	assembler.clear();
	EXPECT_EQ(budget->used(), 0u);
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
	assembler.clear();
	EXPECT_EQ(budget->used(), 0u);
	scheduler.clear();
	EXPECT_EQ(scheduler.pendingTransfers(), 0u);
	PageScheduler nextScheduler;
	(void)nextScheduler.enqueue(message(64, 82));
	const auto second = nextScheduler.nextPage(16);
	ASSERT_TRUE(second.has_value());
	EXPECT_FALSE(assembler.accept(fragment(*second)).has_value());
	EXPECT_GT(budget->used(), 64u);
	assembler.clear();
	EXPECT_EQ(budget->used(), 0u);
}

TEST(RetiredTransferIDs, CompressesSequentialIDsWithoutRetainingIndividualTombstones)
{
	spk::NetworkInternal::RetiredTransferIDs retired;
	for (std::uint64_t id = 1; id <= 100000; ++id)
	{
		retired.retire(id);
	}
	EXPECT_EQ(retired.rangeCount(), 0u);
	EXPECT_TRUE(retired.contains(1));
	EXPECT_TRUE(retired.contains(512));
	EXPECT_TRUE(retired.contains(100000));
	EXPECT_FALSE(retired.contains(100001));
	retired.clear();
	EXPECT_FALSE(retired.contains(1));
	EXPECT_EQ(retired.rangeCount(), 0u);
}

TEST(RetiredTransferIDs, MergesOutOfOrderRangesWithoutForgettingHoles)
{
	spk::NetworkInternal::RetiredTransferIDs retired;
	retired.retire(2);
	retired.retire(4);
	EXPECT_EQ(retired.rangeCount(), 2u);
	EXPECT_FALSE(retired.contains(1));
	EXPECT_FALSE(retired.contains(3));
	retired.retire(3);
	EXPECT_EQ(retired.rangeCount(), 1u);
	retired.retire(1);
	EXPECT_EQ(retired.rangeCount(), 0u);
	for (std::uint64_t id = 1; id <= 4; ++id)
	{
		EXPECT_TRUE(retired.contains(id));
	}
}

TEST(RetiredTransferIDs, HandlesMaximumIDAndDuplicateRetirementWithoutOverflow)
{
	spk::NetworkInternal::RetiredTransferIDs retired;
	const auto maximum = std::numeric_limits<std::uint64_t>::max();
	retired.retire(maximum);
	retired.retire(maximum - 1);
	retired.retire(maximum);
	EXPECT_EQ(retired.rangeCount(), 1u);
	EXPECT_TRUE(retired.contains(maximum));
	EXPECT_TRUE(retired.contains(maximum - 1));
	EXPECT_FALSE(retired.contains(maximum - 2));
	EXPECT_FALSE(retired.contains(0));
}

TEST(PageReassembler, RetiredTransferReplaysStaySuppressedBeyondTombstoneTimeoutAndCapacity)
{
	using namespace spk::NetworkInternal;
	PageReassembler assembler(16, 4096, std::chrono::milliseconds(10));
	const auto original = message(1, 211);
	const auto originalPage = PageFragment{original.header(), 1, 1, 0, original.data()};
	const auto now = PageReassembler::Clock::now();
	ASSERT_TRUE(assembler.accept(originalPage, now).has_value());
	for (std::uint64_t id = 2; id <= 2048; ++id)
	{
		const auto page = PageFragment{original.header(), id, 1, 0, original.data()};
		ASSERT_TRUE(assembler.accept(page, now).has_value());
	}
	EXPECT_TRUE(assembler.expire(now + std::chrono::hours(24)).empty());
	EXPECT_FALSE(assembler.accept(originalPage, now + std::chrono::hours(24)).has_value());
	EXPECT_EQ(assembler.reservedBytes(), 0u);
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
}

TEST(PageReassembler, ExpiredAndFailedTransferIDsRemainProtectedForSessionLifetime)
{
	using namespace spk::NetworkInternal;
	PageReassembler assembler(32, 4096, std::chrono::milliseconds(10));
	const auto original = message(4, 212);
	const auto now = PageReassembler::Clock::now();
	const auto first = PageFragment{original.header(), 1, 4, 0, original.data().first(2)};
	EXPECT_FALSE(assembler.accept(first, now).has_value());
	EXPECT_EQ(assembler.expire(now + std::chrono::milliseconds(11)), (std::vector<std::uint64_t>{1}));
	EXPECT_FALSE(assembler.accept(first, now + std::chrono::hours(24)).has_value());
	const auto failed = PageFragment{original.header(), 2, 4, 1, original.data().first(2)};
	EXPECT_THROW((void)assembler.accept(failed, now), PageReassemblyError);
	const auto valid = PageFragment{original.header(), 2, 4, 0, original.data().first(2)};
	EXPECT_FALSE(assembler.accept(valid, now + std::chrono::hours(24)).has_value());
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
}

TEST(PageReassembler, RetiredIDsAreScopedToOneSessionAndResetOnClear)
{
	using namespace spk::NetworkInternal;
	PageReassembler first;
	PageReassembler second;
	const auto original = message(1, 213);
	const PageFragment page{original.header(), 1, original.size(), 0, original.data()};
	ASSERT_TRUE(first.accept(page).has_value());
	EXPECT_FALSE(first.accept(page).has_value());
	ASSERT_TRUE(second.accept(page).has_value());
	first.clear();
	EXPECT_TRUE(first.accept(page).has_value());
}

TEST(RetiredTransferIDs, SparseHistoryIsBoundedWithoutExpiringOrForgettingIDs)
{
	using spk::NetworkInternal::RetiredTransferIDs;
	RetiredTransferIDs retired;
	for (std::uint64_t index = 1; index <= RetiredTransferIDs::MaximumSparseRanges; ++index)
	{
		retired.retire(index * 2);
	}
	EXPECT_EQ(retired.rangeCount(), RetiredTransferIDs::MaximumSparseRanges);
	EXPECT_THROW(retired.retire((RetiredTransferIDs::MaximumSparseRanges + 1) * 2), spk::Exception);
	EXPECT_TRUE(retired.contains(2));
	EXPECT_TRUE(retired.contains(RetiredTransferIDs::MaximumSparseRanges * 2));
	EXPECT_FALSE(retired.contains((RetiredTransferIDs::MaximumSparseRanges + 1) * 2));
	retired.retire(1);
	EXPECT_EQ(retired.rangeCount(), RetiredTransferIDs::MaximumSparseRanges - 1);
	retired.retire((RetiredTransferIDs::MaximumSparseRanges + 1) * 2);
	EXPECT_EQ(retired.rangeCount(), RetiredTransferIDs::MaximumSparseRanges);
	EXPECT_TRUE(retired.contains(1));
	EXPECT_TRUE(retired.contains(2));
}

TEST(PageReassembler, SparseRetirementExhaustionFailsClosedAndReleasesCompletedPayloads)
{
	using namespace spk::NetworkInternal;
	PageReassembler assembler(16, 4096);
	const auto payload = message(1, 214);
	for (std::uint64_t index = 1; index <= RetiredTransferIDs::MaximumSparseRanges; ++index)
	{
		const PageFragment page{payload.header(), index * 2, 1, 0, payload.data()};
		ASSERT_TRUE(assembler.accept(page).has_value());
	}
	const PageFragment excess{payload.header(), (RetiredTransferIDs::MaximumSparseRanges + 1) * 2, 1, 0, payload.data()};
	EXPECT_THROW((void)assembler.accept(excess), spk::Exception);
	EXPECT_EQ(assembler.pendingTransfers(), 0u);
	EXPECT_EQ(assembler.reservedBytes(), 0u);
}
