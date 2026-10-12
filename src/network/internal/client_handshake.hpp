#pragma once

#include "network/internal/handshake_deadline.hpp"
#include "network/internal/socket.hpp"

#include <cstdint>
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
		};

	private:
		HandshakeDeadline _deadline;
		std::stop_token _stop;
		Connection::ID _id = Connection::InvalidID;
		Connection::Error _error = Connection::Error::Refused;

		void _negotiate(Socket &socket);

	public:
		ClientHandshake(HandshakeDeadline::Clock::time_point expiresAt, std::stop_token stop);
		[[nodiscard]] Result connect(std::string_view address, std::uint16_t port) noexcept;
	};
}
