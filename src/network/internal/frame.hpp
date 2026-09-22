#pragma once

#include "exception.hpp"
#include "network/message.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace spk::NetworkInternal
{
	inline constexpr std::size_t FrameHeaderSize = 16;
	inline constexpr std::uint16_t ProtocolVersion = 1;
	inline constexpr std::uint32_t MaximumPayloadSize = 32u * 1024u * 1024u;

	struct FrameHeader
	{
		Message::Type type;
		std::uint32_t payloadSize;
	};

	inline void write16(std::byte *target, std::uint16_t value)
	{
		target[0] = static_cast<std::byte>(value & 0xFFu);
		target[1] = static_cast<std::byte>((value >> 8u) & 0xFFu);
	}

	inline void write32(std::byte *target, std::uint32_t value)
	{
		for (std::size_t index = 0; index < 4; ++index)
		{
			target[index] = static_cast<std::byte>((value >> (index * 8u)) & 0xFFu);
		}
	}

	[[nodiscard]] inline std::uint16_t read16(const std::byte *source)
	{
		return static_cast<std::uint16_t>(
			std::to_integer<std::uint8_t>(source[0]) |
			(std::to_integer<std::uint8_t>(source[1]) << 8u));
	}

	[[nodiscard]] inline std::uint32_t read32(const std::byte *source)
	{
		std::uint32_t result = 0;
		for (std::size_t index = 0; index < 4; ++index)
		{
			result |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(source[index])) << (index * 8u);
		}
		return result;
	}

	[[nodiscard]] inline std::shared_ptr<std::vector<std::byte>> encode(const Message &message)
	{
		if (message.size() > MaximumPayloadSize)
		{
			throw Exception("Network message exceeds the maximum payload size.");
		}

		auto result = std::make_shared<std::vector<std::byte>>(FrameHeaderSize + message.size());
		auto *bytes = result->data();
		bytes[0] = std::byte{'S'};
		bytes[1] = std::byte{'P'};
		bytes[2] = std::byte{'K'};
		bytes[3] = std::byte{'N'};
		write16(bytes + 4, ProtocolVersion);
		write16(bytes + 6, 0);
		write32(bytes + 8, message.type());
		write32(bytes + 12, static_cast<std::uint32_t>(message.size()));
		std::copy(message.data().begin(), message.data().end(), result->begin() + FrameHeaderSize);
		return result;
	}

	[[nodiscard]] inline FrameHeader decode(std::span<const std::byte, FrameHeaderSize> bytes)
	{
		if (bytes[0] != std::byte{'S'} || bytes[1] != std::byte{'P'} ||
			bytes[2] != std::byte{'K'} || bytes[3] != std::byte{'N'})
		{
			throw Exception("Invalid Sparkle network frame magic.");
		}
		if (read16(bytes.data() + 4) != ProtocolVersion)
		{
			throw Exception("Unsupported Sparkle network protocol version.");
		}

		const std::uint32_t payloadSize = read32(bytes.data() + 12);
		if (payloadSize > MaximumPayloadSize)
		{
			throw Exception("Network frame payload exceeds the configured limit.");
		}
		return FrameHeader{read32(bytes.data() + 8), payloadSize};
	}
}
