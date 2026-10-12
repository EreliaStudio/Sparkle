#pragma once

#include "network/internal/handshake.hpp"

#include <chrono>

namespace spk::NetworkInternal
{
	class HandshakeDeadline final
	{
	public:
		using Clock = std::chrono::steady_clock;

	private:
		Clock::time_point _expiresAt;

	public:
		explicit HandshakeDeadline(
			std::chrono::milliseconds duration = Handshake::DefaultTimeout,
			Clock::time_point now = Clock::now());
		[[nodiscard]] Clock::time_point expiresAt() const noexcept;
		[[nodiscard]] bool expired(Clock::time_point now = Clock::now()) const noexcept;
		[[nodiscard]] std::chrono::milliseconds remaining(Clock::time_point now = Clock::now()) const noexcept;
	};
}
