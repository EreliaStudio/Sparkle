#pragma once

#include "network/internal/handshake_deadline.hpp"

#include <span>

namespace spk::NetworkInternal
{
	class ServerHandshake final
	{
	public:
		enum class State
		{
			Pending,
			Accepting,
			Ready,
			Rejecting,
			Rejected
		};

	private:
		HandshakeDeadline _deadline;
		State _state = State::Pending;
		Connection::Error _error = Connection::Error::None;
		std::uint32_t _maximumPageBytes = DefaultMaximumPageBytes;

	public:
		explicit ServerHandshake(std::chrono::milliseconds timeout = Handshake::DefaultTimeout, std::uint32_t maximumPageBytes = DefaultMaximumPageBytes);
		[[nodiscard]] Handshake::Kind accept(std::span<const std::byte> frame);
		[[nodiscard]] bool written(Handshake::Kind kind);
		[[nodiscard]] State state() const noexcept;
		[[nodiscard]] bool ready() const noexcept;
		[[nodiscard]] bool expired() const noexcept;
		[[nodiscard]] Connection::Error error() const noexcept;
		[[nodiscard]] std::uint32_t maximumPageBytes() const noexcept;
		void fail(Connection::Error error) noexcept;
	};
}
