#pragma once

#include "network/internal/command_budget.hpp"
#include "network/internal/page.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>

namespace spk::NetworkInternal
{
	struct OutgoingReservation final
	{
		std::shared_ptr<CommandBudget::Lease> logical;
		std::shared_ptr<CommandBudget::Lease> frame;

		[[nodiscard]] std::size_t bytes() const noexcept;

		[[nodiscard]] static std::size_t frameBytes(std::size_t messageBytes, std::size_t maxPageBytes) noexcept;

		[[nodiscard]] static OutgoingReservation reserve(CommandBudget &budget, std::size_t messageBytes, std::size_t maxPageBytes);

		[[nodiscard]] static std::optional<OutgoingReservation> tryReserve(CommandBudget &budget, std::size_t messageBytes, std::size_t maxPageBytes);
	};
}
