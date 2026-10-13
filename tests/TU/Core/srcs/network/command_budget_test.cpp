#include "network/internal/command_budget.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <utility>

TEST(CommandBudget, RejectsOnlyNewReservationsAtCapacity)
{
	spk::NetworkInternal::CommandBudget budget(100);
	auto first = budget.reserve(70);
	EXPECT_EQ(budget.used(), 70u);
	EXPECT_THROW((void)budget.reserve(31), spk::Exception);
	EXPECT_EQ(budget.used(), 70u);
	auto second = budget.reserve(30);
	EXPECT_EQ(budget.used(), 100u);
	second.reset();
	EXPECT_EQ(budget.used(), 70u);
	first.reset();
	EXPECT_EQ(budget.used(), 0u);
}

TEST(CommandBudget, LoweredLimitDoesNotEvictExistingReservations)
{
	spk::NetworkInternal::CommandBudget budget(100);
	auto first = budget.reserve(80);
	budget.setMaximum(50);
	EXPECT_EQ(budget.used(), 80u);
	EXPECT_THROW((void)budget.reserve(1), spk::Exception);
	first.reset();
	EXPECT_EQ(budget.used(), 0u);
	auto second = budget.reserve(50);
	EXPECT_EQ(budget.used(), 50u);
}

TEST(CommandBudget, TracksPendingCommandsAndActiveTransfersUnderOneLimit)
{
	using Budget = spk::NetworkInternal::CommandBudget;
	Budget budget(128);
	auto pending = budget.reserve(64);
	auto active = budget.reserve(48);
	active->transition(Budget::Stage::Transfer);
	EXPECT_EQ(budget.used(), 112u);
	EXPECT_EQ(budget.used(Budget::Stage::Command), 64u);
	EXPECT_EQ(budget.used(Budget::Stage::Transfer), 48u);
	EXPECT_THROW((void)budget.reserve(17), spk::Exception);
	pending.reset();
	EXPECT_EQ(budget.used(), 48u);
	active.reset();
	EXPECT_EQ(budget.used(), 0u);
}

TEST(CommandBudget, FrameReservationsRemainChargedDuringPartialWrites)
{
	using Budget = spk::NetworkInternal::CommandBudget;
	Budget budget(100);
	auto first = budget.reserve(60, Budget::Stage::Frame);
	budget.setMaximum(30);
	EXPECT_EQ(budget.used(Budget::Stage::Frame), 60u);
	EXPECT_THROW((void)budget.reserve(1), spk::Exception);
	first.reset();
	EXPECT_EQ(budget.used(Budget::Stage::Frame), 0u);
	EXPECT_EQ(budget.used(), 0u);
}

TEST(CommandBudget, FrameReservationsTransitionToInFlightWithoutChangingTotal)
{
	using Budget = spk::NetworkInternal::CommandBudget;
	Budget budget(128);
	auto logical = budget.reserve(64, Budget::Stage::Transfer);
	auto frame = budget.reserve(60, Budget::Stage::Frame);
	EXPECT_EQ(budget.used(), 124u);
	frame->transition(Budget::Stage::InFlight);
	EXPECT_EQ(budget.used(Budget::Stage::Frame), 0u);
	EXPECT_EQ(budget.used(Budget::Stage::InFlight), 60u);
	EXPECT_THROW((void)budget.reserve(5), spk::Exception);
	frame->transition(Budget::Stage::Frame);
	EXPECT_EQ(budget.used(Budget::Stage::InFlight), 0u);
	EXPECT_EQ(budget.used(Budget::Stage::Frame), 60u);
	frame.reset();
	logical.reset();
	EXPECT_EQ(budget.used(), 0u);
}

TEST(CommandBudget, ReservationCanOutliveBudgetOwnerAndReleasesSafely)
{
	using Budget = spk::NetworkInternal::CommandBudget;
	std::shared_ptr<Budget::Lease> lease;
	{
		Budget budget(64);
		lease = budget.reserve(60, Budget::Stage::Frame);
		EXPECT_EQ(budget.used(), 60u);
	}
	EXPECT_EQ(lease->bytes(), 60u);
	lease.reset();
}
