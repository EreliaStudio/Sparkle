#include "network/internal/outgoing_reservation.hpp"

#include "exception.hpp"

#include <gtest/gtest.h>

TEST(OutgoingReservation, ReservesLogicalAndFrameMemoryAtomicallyWithRAIICleanup)
{
	using spk::NetworkInternal::CommandBudget;
	using spk::NetworkInternal::OutgoingReservation;
	CommandBudget budget(256);
	{
		auto result = OutgoingReservation::reserve(budget, 100, 32);
		EXPECT_EQ(result.bytes(), 100u + spk::NetworkInternal::FrameHeaderSize + spk::NetworkInternal::PageMetadataSize + 32u);
		EXPECT_EQ(budget.used(), result.bytes());
		EXPECT_EQ(budget.used(CommandBudget::Stage::Transfer), 100u);
		EXPECT_EQ(budget.used(CommandBudget::Stage::Frame), result.frame->bytes());
	}
	EXPECT_EQ(budget.used(), 0u);
}

TEST(OutgoingReservation, InsufficientFrameHeadroomDoesNotLeakLogicalReservation)
{
	using spk::NetworkInternal::CommandBudget;
	using spk::NetworkInternal::OutgoingReservation;
	CommandBudget budget(120);
	EXPECT_THROW((void)OutgoingReservation::reserve(budget, 100, 32), spk::Exception);
	EXPECT_EQ(budget.used(), 0u);
}

TEST(OutgoingReservation, EmptyMessageStillReservesFullPageHeader)
{
	using spk::NetworkInternal::CommandBudget;
	using spk::NetworkInternal::OutgoingReservation;
	CommandBudget budget(256);
	auto reservation = OutgoingReservation::reserve(budget, 0, 32);
	EXPECT_EQ(reservation.logical->bytes(), 1u);
	EXPECT_EQ(reservation.frame->bytes(), spk::NetworkInternal::FrameHeaderSize + spk::NetworkInternal::PageMetadataSize);
}

TEST(OutgoingReservation, TryReserveReturnsEmptyAndRollsBackInsufficientFrameHeadroom)
{
	using spk::NetworkInternal::CommandBudget;
	using spk::NetworkInternal::OutgoingReservation;
	CommandBudget budget(120);
	EXPECT_FALSE(OutgoingReservation::tryReserve(budget, 100, 32).has_value());
	EXPECT_EQ(budget.used(), 0u);
	const auto recovered = OutgoingReservation::tryReserve(budget, 1, 32);
	ASSERT_TRUE(recovered.has_value());
	EXPECT_EQ(budget.used(), recovered->bytes());
}
