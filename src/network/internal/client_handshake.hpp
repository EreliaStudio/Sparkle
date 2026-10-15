#pragma once

#include "network/internal/handshake_deadline.hpp"
#include "network/internal/socket.hpp"

#include <cstdint>
#include <span>
#include <stop_token>
#include <string_view>

namespace spk::NetworkInternal
{
	class ClientHandshake final
	{
	public:
		struct Result final
		{
			Socket socket;
			Connection::ID id = Connection::InvalidID;
			Connection::Error error = Connection::Error::None;
			std::uint32_t maximumPageBytes = DefaultMaximumPageBytes;
		};

	private:
		HandshakeDeadline _deadline;
		std::stop_token _stop;
		Connection::ID _id = Connection::InvalidID;
		Connection::Error _error = Connection::Error::Refused;
		std::uint32_t _requestedPageBytes = DefaultMaximumPageBytes;
		std::uint32_t _negotiatedPageBytes = DefaultMaximumPageBytes;

		void _validateHeader(std::span<const std::byte, FrameHeaderSize> header);
		[[nodiscard]] Handshake::Packet _readReply(Socket &socket);
		void _validateReply(const Handshake::Packet &packet);
		void _negotiate(Socket &socket);

	public:
		ClientHandshake(HandshakeDeadline::Clock::time_point expiresAt, std::stop_token stop, std::uint32_t maximumPageBytes = DefaultMaximumPageBytes);
		[[nodiscard]] Result connect(std::string_view address, std::uint16_t port) noexcept;
	};
}
