#include "network/internal/client_handshake.hpp"

#include "exception.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/handshake.hpp"

#include <array>
#include <utility>

namespace spk::NetworkInternal
{
	ClientHandshake::ClientHandshake(HandshakeDeadline::Clock::time_point expiresAt, std::stop_token stop, std::uint32_t maximumPageBytes) :
		_deadline(expiresAt),
		_stop(stop),
		_requestedPageBytes(maximumPageBytes)
	{
	}

	void ClientHandshake::_validateHeader(std::span<const std::byte, FrameHeaderSize> header)
	{
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
	}

	Handshake::Packet ClientHandshake::_readReply(Socket &socket)
	{
		std::array<std::byte, FrameHeaderSize + Handshake::PayloadSize> frame{};
		auto header = std::span(frame).first<FrameHeaderSize>();
		if (!socket.receiveAllUntil(header, _deadline.expiresAt(), _stop))
		{
			throw Exception("Unable to receive handshake header.");
		}
		_validateHeader(header);
		auto payload = std::span(frame).last<Handshake::PayloadSize>();
		if (!socket.receiveAllUntil(payload, _deadline.expiresAt(), _stop))
		{
			throw Exception("Incomplete handshake response.");
		}
		return Handshake::decode(frame);
	}

	void ClientHandshake::_validateReply(const Handshake::Packet &packet)
	{
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
		if (packet.maximumPageBytes > _requestedPageBytes)
		{
			_error = Connection::Error::Rejected;
			throw Exception("Server selected an unsupported page size.");
		}
		_id = packet.connectionID;
		_negotiatedPageBytes = packet.maximumPageBytes;
	}

	void ClientHandshake::_negotiate(Socket &socket)
	{
		if (!socket.sendAllUntil(*Handshake::encode(Handshake::Kind::Hello, Connection::InvalidID, Connection::Error::None, _requestedPageBytes), _deadline.expiresAt(), _stop))
		{
			throw Exception("Unable to send handshake request before deadline.");
		}
		_validateReply(_readReply(socket));
	}

	ClientHandshake::Result ClientHandshake::connect(std::string_view address, std::uint16_t port) noexcept
	{
		try
		{
			Socket socket = Socket::connectTCP(address, port, _deadline.expiresAt(), _stop);
			_error = Connection::Error::TransportFailure;
			_negotiate(socket);
			return {std::move(socket), _id, Connection::Error::None, _negotiatedPageBytes};
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
