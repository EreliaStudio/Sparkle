#include "network/internal/handshake.hpp"

#include "exception.hpp"

#include <algorithm>

namespace spk::NetworkInternal
{
	std::shared_ptr<std::vector<std::byte>> Handshake::encode(Kind kind, Connection::ID id, Connection::Error error)
	{
		if ((kind == Kind::Hello && (id != Connection::InvalidID || error != Connection::Error::None)) ||
			(kind == Kind::Accepted && (id == Connection::InvalidID || error != Connection::Error::None)) ||
			(kind == Kind::Rejected && error == Connection::Error::None))
		{
			throw Exception("Invalid handshake control frame.");
		}
		auto frame = std::make_shared<std::vector<std::byte>>(FrameHeaderSize + PayloadSize);
		auto *data = frame->data();
		data[0] = std::byte{'S'};
		data[1] = std::byte{'P'};
		data[2] = std::byte{'K'};
		data[3] = std::byte{'N'};
		write16(data + 4, ProtocolVersion);
		write16(data + 6, static_cast<std::uint16_t>(kind));
		write32(data + 8, 0);
		write64(data + 12, 0);
		write64(data + 20, id);
		write32(data + 28, PayloadSize);
		data[32] = std::byte{'S'};
		data[33] = std::byte{'P'};
		data[34] = std::byte{'K'};
		data[35] = std::byte{'H'};
		data[36] = std::byte{0};
		data[37] = std::byte{1};
		data[38] = std::byte{0};
		data[39] = static_cast<std::byte>(error);
		return frame;
	}

	Handshake::Packet Handshake::decode(std::span<const std::byte> bytes)
	{
		if (bytes.size() != FrameHeaderSize + PayloadSize)
		{
			throw Exception("Invalid handshake frame length.");
		}
		const auto header = NetworkInternal::decode(
			std::span<const std::byte, FrameHeaderSize>(bytes.data(), FrameHeaderSize));
		const auto kind = static_cast<Kind>(read16(bytes.data() + 6));
		if ((kind != Kind::Hello && kind != Kind::Accepted && kind != Kind::Rejected) ||
			header.payloadSize != PayloadSize || header.type != 0 || header.requestID != 0)
		{
			throw Exception("Invalid handshake frame metadata.");
		}
		const auto error = static_cast<Connection::Error>(std::to_integer<std::uint8_t>(bytes[39]));
		Packet packet{kind, header.connectionID, error};
		if (bytes[32] != std::byte{'S'} || bytes[33] != std::byte{'P'} ||
			bytes[34] != std::byte{'K'} || bytes[35] != std::byte{'H'})
		{
			packet.validationError = Connection::Error::InvalidMagic;
		}
		else if (bytes[36] != std::byte{0} || bytes[37] != std::byte{1} || bytes[38] != std::byte{0})
		{
			packet.validationError = Connection::Error::VersionMismatch;
		}
		return packet;
	}

	bool Handshake::validHello(const Packet &packet) noexcept
	{
		return packet.kind == Kind::Hello && packet.connectionID == Connection::InvalidID && packet.error == Connection::Error::None && packet.validationError == Connection::Error::None;
	}

	bool Handshake::validAcceptance(const Packet &packet) noexcept
	{
		return packet.kind == Kind::Accepted && packet.connectionID != Connection::InvalidID && packet.error == Connection::Error::None && packet.validationError == Connection::Error::None;
	}
}
