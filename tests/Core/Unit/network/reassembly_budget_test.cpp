#include "network/internal/reassembly_budget.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <barrier>
#include <memory>
#include <thread>
#include <vector>

namespace
{
	using spk::NetworkInternal::ReassemblyBudget;
}

TEST(ReassemblyBudget, ReductionPreservesExistingAllowanceWithoutLendingItToNewLeases)
{
	ReassemblyBudget budget(100);
	auto existing = budget.reserve(10);
	ASSERT_NE(existing, nullptr);
	budget.setMaximum(50);
	auto newer = budget.reserve(10);
	ASSERT_NE(newer, nullptr);
	EXPECT_FALSE(newer->reserve(31));
	EXPECT_TRUE(existing->reserve(31));
	EXPECT_EQ(budget.used(), 51u);
	EXPECT_EQ(budget.reserve(1), nullptr);
	EXPECT_FALSE(newer->reserve(1));
	existing.reset();
	EXPECT_EQ(budget.used(), 10u);
	EXPECT_TRUE(newer->reserve(40));
	EXPECT_FALSE(newer->reserve(1));
	newer.reset();
	EXPECT_EQ(budget.used(), 0u);
}

TEST(ReassemblyBudget, RetainedAllowanceStillBoundsContinuationReservations)
{
	ReassemblyBudget budget(100);
	auto lease = budget.reserve(10);
	ASSERT_NE(lease, nullptr);
	budget.setMaximum(1);
	EXPECT_TRUE(lease->reserve(90));
	EXPECT_FALSE(lease->reserve(1));
	EXPECT_EQ(budget.used(), 100u);
	lease->release(90);
	EXPECT_EQ(budget.used(), 10u);
	lease.reset();
	EXPECT_EQ(budget.used(), 0u);
	EXPECT_EQ(budget.reserve(2), nullptr);
	EXPECT_NE(budget.reserve(1), nullptr);
	EXPECT_EQ(budget.used(), 0u);
}

TEST(ReassemblyBudget, RaisedAllowancesSurviveLaterReductions)
{
	ReassemblyBudget budget(100);
	auto first = budget.reserve(20);
	ASSERT_NE(first, nullptr);
	budget.setMaximum(500);
	auto second = budget.reserve(250);
	ASSERT_NE(second, nullptr);
	budget.setMaximum(50);
	EXPECT_TRUE(first->reserve(100));
	EXPECT_TRUE(second->reserve(100));
	EXPECT_EQ(budget.used(), 470u);
	EXPECT_FALSE(first->reserve(31));
	first.reset();
	second.reset();
	EXPECT_EQ(budget.used(), 0u);
	EXPECT_EQ(budget.reserve(51), nullptr);
	EXPECT_NE(budget.reserve(50), nullptr);
}

TEST(ReassemblyBudget, LeaseOwnsAccountingUntilItIsDestroyed)
{
	std::unique_ptr<ReassemblyBudget::Lease> lease;
	{
		ReassemblyBudget budget(100);
		lease = budget.reserve(10);
		ASSERT_NE(lease, nullptr);
		EXPECT_EQ(budget.used(), 10u);
	}
	EXPECT_TRUE(lease->reserve(90));
	EXPECT_FALSE(lease->reserve(1));
	lease.reset();
}

TEST(ReassemblyBudget, InvalidLimitDoesNotChangeAdmissionOrExistingAllowances)
{
	EXPECT_THROW(ReassemblyBudget(0), spk::Exception);
	ReassemblyBudget budget(100);
	auto lease = budget.reserve(10);
	ASSERT_NE(lease, nullptr);
	EXPECT_THROW(budget.setMaximum(0), spk::Exception);
	EXPECT_TRUE(lease->reserve(90));
	lease.reset();
	EXPECT_NE(budget.reserve(100), nullptr);
}

TEST(ReassemblyBudget, ResizeBehaviourRejectsUnknownValue)
{
	ReassemblyBudget budget(100);
	auto lease = budget.reserve(10);
	ASSERT_NE(lease, nullptr);
	EXPECT_THROW(budget.setMaximum(1, static_cast<ReassemblyBudget::ResizeBehaviour>(99)), spk::Exception);
	EXPECT_NO_THROW(budget.setMaximum(50, ReassemblyBudget::ResizeBehaviour::PreserveAcceptedLeases));
	EXPECT_TRUE(lease->reserve(90));
	lease.reset();
}

TEST(ReassemblyBudget, RuntimeLimitsCanRaceLeaseAdmissionGrowthAndDestruction)
{
	constexpr std::size_t Count = 4;
	ReassemblyBudget budget(4096);
	std::barrier start(Count + 1);
	std::atomic_bool valid = true;
	std::vector<std::jthread> workers;
	for (std::size_t index = 0; index < Count; ++index)
	{
		workers.emplace_back([&] {
			auto initial = budget.reserve(32);
			if (initial == nullptr)
			{
				valid = false;
			}
			start.arrive_and_wait();
			initial.reset();
			for (std::size_t attempt = 0; attempt < 1000; ++attempt)
			{
				auto lease = budget.reserve(32);
				if (lease == nullptr)
				{
					continue;
				}
				if (lease->reserve(8))
				{
					lease->release(8);
				}
				else
				{
					valid = false;
				}
			}
		});
	}
	start.arrive_and_wait();
	for (std::size_t attempt = 0; attempt < 2000; ++attempt)
	{
		budget.setMaximum(attempt % 2 == 0 ? 1 : 4096);
	}
	for (std::jthread &worker : workers)
	{
		worker.join();
	}
	EXPECT_TRUE(valid.load());
	EXPECT_EQ(budget.used(), 0u);
}
