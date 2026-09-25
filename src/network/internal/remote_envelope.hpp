#pragma once

#include "exception.hpp"
#include "network/message.hpp"
#include "network/types.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace spk::NetworkInternal
{
	inline constexpr Message::Type RemoteEnvelopeType = 0xFFFFFFFEu;
	inline constexpr std::size_t RemoteEnvelopeHeaderSize = 24;
	inline constexpr std::uint16_t RemoteEnvelopeVersion = 1;
	inline constexpr std::uint32_t RemoteEnvelopeMagic = 0x444F4E52u;

	enum class RemoteEnvelopeKind : std::uint16_t
	{
		Request = 1,
		Response = 2
	};

	struct RemoteEnvelope
	{
		RemoteEnvelopeKind kind;
		ConnectionID route;
		Message message;
	};

	inline void writeRemote16(std::byte *target, std::uint16_t value)
	{
		for (std::size_t index = 0; index < 2; ++index)
		{
			target[index] = static_cast<std::byte>((value >> (index * 8u)) & 0xFFu);
		}
	}

	inline void writeRemote32(std::byte *target, std::uint32_t value)
	{
		for (std::size_t index = 0; index < 4; ++index)
		{
			target[index] = static_cast<std::byte>((value >> (index * 8u)) & 0xFFu);
		}
	}

	inline void writeRemote64(std::byte *target, std::uint64_t value)
	{
		for (std::size_t index = 0; index < 8; ++index)
		{
			target[index] = static_cast<std::byte>((value >> (index * 8u)) & 0xFFu);
		}
	}

	[[nodiscard]] inline std::uint16_t readRemote16(const std::byte *source)
	{
		std::uint16_t result = 0;
		for (std::size_t index = 0; index < 2; ++index)
		{
			result |= static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(source[index])) << (index * 8u);
		}
		return result;
	}

	[[nodiscard]] inline std::uint32_t readRemote32(const std::byte *source)
	{
		std::uint32_t result = 0;
		for (std::size_t index = 0; index < 4; ++index)
		{
			result |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(source[index])) << (index * 8u);
		}
		return result;
	}

	[[nodiscard]] inline std::uint64_t readRemote64(const std::byte *source)
	{
		std::uint64_t result = 0;
		for (std::size_t index = 0; index < 8; ++index)
		{
			result |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(source[index])) << (index * 8u);
		}
		return result;
	}

	[[nodiscard]] inline Message encodeRemoteEnvelope(
		RemoteEnvelopeKind kind,
		ConnectionID route,
		const Message &message)
	{
		Message::Storage payload(RemoteEnvelopeHeaderSize + message.size());
		writeRemote32(payload.data(), RemoteEnvelopeMagic);
		writeRemote16(payload.data() + 4, RemoteEnvelopeVersion);
		writeRemote16(payload.data() + 6, static_cast<std::uint16_t>(kind));
		writeRemote64(payload.data() + 8, route);
		writeRemote32(payload.data() + 16, message.type());
		writeRemote32(payload.data() + 20, static_cast<std::uint32_t>(message.size()));
		std::copy(message.data().begin(), message.data().end(), payload.begin() + RemoteEnvelopeHeaderSize);
		Message envelope(RemoteEnvelopeType, std::move(payload));
		envelope.setRequestID(message.requestID());
		return envelope;
	}

	[[nodiscard]] inline RemoteEnvelope decodeRemoteEnvelope(const Message &message)
	{
		if (message.type() != RemoteEnvelopeType || message.size() < RemoteEnvelopeHeaderSize)
		{
			throw Exception("Invalid remote node envelope.");
		}

		const auto data = message.data();
		if (readRemote32(data.data()) != RemoteEnvelopeMagic ||
			readRemote16(data.data() + 4) != RemoteEnvelopeVersion)
		{
			throw Exception("Unsupported remote node envelope.");
		}

		const std::uint32_t payloadSize = readRemote32(data.data() + 20);
		if (payloadSize != message.size() - RemoteEnvelopeHeaderSize)
		{
			throw Exception("Invalid remote node envelope payload size.");
		}

		Message::Storage payload(payloadSize);
		std::copy(
			data.begin() + RemoteEnvelopeHeaderSize,
			data.end(),
			payload.begin());
		Message decoded(readRemote32(data.data() + 16), std::move(payload));
		decoded.setRequestID(message.requestID());
		return RemoteEnvelope{
			static_cast<RemoteEnvelopeKind>(readRemote16(data.data() + 6)),
			readRemote64(data.data() + 8),
			std::move(decoded)};
	}
}
