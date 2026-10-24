#pragma once

#include "network/internal/frame.hpp"
#include "network/internal/transfer_reception.hpp"

#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

namespace spk::NetworkInternal
{
	class ClientReception final
	{
	public:
		struct Result
		{
			std::optional<Message> message;
			std::optional<TransferResponse> response;
			std::optional<TransferResponse> acknowledgment;
		};

	private:
		mutable std::mutex _mutex;
		TransferReception _reception;

	public:
		[[nodiscard]] Result accept(const FrameHeader &header, std::span<const std::byte> frame);
		[[nodiscard]] std::vector<TransferResponse> expire(Connection::ID id);
		void setInactivityTimeout(std::chrono::milliseconds timeout);
		void setLimits(std::size_t maximumMessageBytes, std::size_t maximumIncompleteBytes);
		void configureFailureLimits(PageFailureLimits limits);
		void clear() noexcept;
	};
}
