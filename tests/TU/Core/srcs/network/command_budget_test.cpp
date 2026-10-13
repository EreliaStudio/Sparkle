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
