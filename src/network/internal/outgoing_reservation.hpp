#pragma once

#include "network/internal/command_budget.hpp"
#include "network/internal/page.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>

namespace spk::NetworkInternal
{
	struct OutgoingReservation final
	{
		std::shared_ptr<CommandBudget::Lease> logical;
		std::shared_ptr<CommandBudget::Lease> frame;

		[[nodiscard]] std::size_t bytes() const noexcept
		{
			return (logical == nullptr ? 0 : logical->bytes()) + (frame == nullptr ? 0 : frame->bytes());
		}

		[[nodiscard]] static std::size_t frameBytes(std::size_t messageBytes, std::size_t maxPageBytes) noexcept
		{
			return FrameHeaderSize + PageMetadataSize + std::min(messageBytes, maxPageBytes);
		}

		[[nodiscard]] static OutgoingReservation reserve(CommandBudget &budget, std::size_t messageBytes, std::size_t maxPageBytes)
		{
			OutgoingReservation result;
			result.logical = budget.reserve(std::max<std::size_t>(1, messageBytes), CommandBudget::Stage::Transfer);
			result.frame = budget.reserve(frameBytes(messageBytes, maxPageBytes), CommandBudget::Stage::Frame);
			return result;
		}
	};
}
