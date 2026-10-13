#pragma once

#include "network/internal/page_reassembler.hpp"
#include "network/internal/transfer_response.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace spk::NetworkInternal
{
	class TransferReception final
	{
	public:
		using Clock = PageReassembler::Clock;

		struct Result final
		{
			std::optional<Message> message;
			std::optional<TransferResponse> response;
		};

	private:
		PageReassembler _reassembler;

		[[nodiscard]] static spk::Transfer::Reason _reason(PageReassemblyError::Reason reason) noexcept;

	public:
		explicit TransferReception(std::shared_ptr<ReassemblyBudget> sharedBudget = {});
		[[nodiscard]] Result accept(const PageFragment &page, Connection::ID connectionID, Clock::time_point now = Clock::now());
		[[nodiscard]] std::vector<TransferResponse> expire(Connection::ID connectionID, Clock::time_point now = Clock::now());
		void configureFailureLimits(PageFailureLimits limits);
		void setInactivityTimeout(std::chrono::milliseconds timeout);
		void setLimits(std::size_t maximumMessageBytes, std::size_t maximumIncompleteBytes);
		[[nodiscard]] bool failureLimitReached() const;
		void successfulMessage();
		void clear() noexcept;
		[[nodiscard]] std::size_t reservedBytes() const noexcept;
	};
}
