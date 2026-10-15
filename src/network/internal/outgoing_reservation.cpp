#include "network/internal/outgoing_reservation.hpp"

#include "exception.hpp"

#include <algorithm>
#include <utility>

namespace spk::NetworkInternal
{
	std::size_t OutgoingReservation::bytes() const noexcept
	{
		return (logical == nullptr ? 0 : logical->bytes()) + (frame == nullptr ? 0 : frame->bytes());
	}

	std::size_t OutgoingReservation::frameBytes(std::size_t messageBytes, std::size_t maxPageBytes) noexcept
	{
		return FrameHeaderSize + PageMetadataSize + std::min(messageBytes, maxPageBytes);
	}

	OutgoingReservation OutgoingReservation::reserve(CommandBudget &budget, std::size_t messageBytes, std::size_t maxPageBytes)
	{
		auto result = tryReserve(budget, messageBytes, maxPageBytes);
		if (!result.has_value())
		{
			throw Exception("Network outgoing memory budget is saturated.");
		}
		return std::move(*result);
	}

	std::optional<OutgoingReservation> OutgoingReservation::tryReserve(CommandBudget &budget, std::size_t messageBytes, std::size_t maxPageBytes)
	{
		OutgoingReservation result;
		result.logical = budget.tryReserve(std::max<std::size_t>(1, messageBytes), CommandBudget::Stage::Transfer);
		if (result.logical == nullptr)
		{
			return std::nullopt;
		}
		result.frame = budget.tryReserve(frameBytes(messageBytes, maxPageBytes), CommandBudget::Stage::Frame);
		if (result.frame == nullptr)
		{
			return std::nullopt;
		}
		return result;
	}
}
