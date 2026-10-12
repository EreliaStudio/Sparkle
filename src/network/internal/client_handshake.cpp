#include "network/internal/client_handshake.hpp"

#include "exception.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/handshake.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace spk::NetworkInternal
{
	ClientHandshake::ClientHandshake(HandshakeDeadline::Clock::time_point expiresAt, std::stop_token stop) :
		_deadline(expiresAt),
		_stop(stop)
	{
	}

	void ClientHandshake::_negotiate(Socket &socket)
	{
		socket.sendAll(*Handshake::encode(Handshake::Kind::Hello));
		std::array<std::byte, FrameHeaderSize> header{};
		if (!socket.receiveAllUntil(header, _deadline.expiresAt(), _stop))
		{
			throw Exception("Unable to receive the handshake header.");
		}
		if (header[0] != std::byte{'S'} || header[1] != std::byte{'P'} ||
			header[2] != std::byte{'K'} || header[3] != std::byte{'N'})
		{
			_error = Connection::Error::InvalidMagic;
			throw Exception("Invalid handshake frame magic.");
		}
		if (read16(header.data() + 4) != ProtocolVersion)
		{
			_error = Connection::Error::VersionMismatch;
			throw Exception("Incompatible network framing version.");
		}
		if (decode(header).payloadSize != Handshake::PayloadSize)
		{
			throw Exception("Invalid handshake response length.");
		}
		std::array<std::byte, Handshake::PayloadSize> payload{};
		if (!socket.receiveAllUntil(payload, _deadline.expiresAt(), _stop))
		{
			throw Exception("Incomplete handshake response.");
		}
		std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> bytes{};
		std::copy(header.begin(), header.end(), bytes.begin());
		std::copy(payload.begin(), payload.end(), bytes.begin() + FrameHeaderSize);
		const auto packet = Handshake::decode(bytes);
		if (packet.validationError != Connection::Error::None)
		{
			_error = packet.validationError;
			throw Exception("Invalid protocol handshake.");
		}
		if (packet.kind == Handshake::Kind::Rejected)
		{
			_error = packet.error == Connection::Error::None ? Connection::Error::Rejected : packet.error;
			throw Exception("Server rejected the network handshake.");
		}
		if (!Handshake::validAcceptance(packet))
		{
			_error = Connection::Error::Rejected;
			throw Exception("Unexpected handshake control frame.");
		}
		_id = packet.connectionID;
	}

	ClientHandshake::Result ClientHandshake::connect(std::string_view address, std::uint16_t port) noexcept
	{
		try
		{
			Socket socket = Socket::connectTCP(address, port, _deadline.expiresAt(), _stop);
			_error = Connection::Error::TransportFailure;
			_negotiate(socket);
			return {std::move(socket), _id, Connection::Error::None};
		} catch (...)
		{
			if (_deadline.expired())
			{
				_error = Connection::Error::Timeout;
			}
			return {{}, Connection::InvalidID, _error};
		}
	}
}
