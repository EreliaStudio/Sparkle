#pragma once

#include "network/internal/frame.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace spk::NetworkInternal
{
	class Handshake final
	{
	public:
		enum class Kind : std::uint16_t
		{
			Hello = 3,
			Accepted = 4,
			Rejected = 5
		};

		struct Packet final
		{
			Kind kind;
			Connection::ID connectionID;
			Connection::Error error = Connection::Error::None;
			Connection::Error validationError = Connection::Error::None;
		};

		inline static constexpr std::size_t PayloadSize = 8;
		inline static constexpr std::chrono::milliseconds DefaultTimeout{5000};

		[[nodiscard]] static std::shared_ptr<std::vector<std::byte>> encode(
			Kind kind,
			Connection::ID id = Connection::InvalidID,
			Connection::Error error = Connection::Error::None);
		[[nodiscard]] static Packet decode(std::span<const std::byte> bytes);
		[[nodiscard]] static bool validHello(const Packet &packet) noexcept;
		[[nodiscard]] static bool validAcceptance(const Packet &packet) noexcept;
	};

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
		[[nodiscard]] bool expired(Clock::time_point now = Clock::now()) const noexcept;
		[[nodiscard]] std::chrono::milliseconds remaining(Clock::time_point now = Clock::now()) const noexcept;
	};
}
