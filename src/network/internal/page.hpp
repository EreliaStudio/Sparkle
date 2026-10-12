#pragma once

#include "exception.hpp"
#include "network/internal/frame.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <vector>

namespace spk::NetworkInternal
{
	inline constexpr std::size_t PageMetadataSize = 24;
	inline constexpr std::size_t DefaultMaximumPageBytes = 64u * 1024u;
	inline constexpr std::uint16_t PageFrameKind = 1;

	struct Page final
	{
		Message message;
		std::uint64_t transferID;
		std::uint64_t totalSize;
		std::uint64_t offset;
		std::size_t length;

		[[nodiscard]] std::span<const std::byte> data() const
		{
			return message.data().subspan(static_cast<std::size_t>(offset), length);
		}
	};

	struct PageFragment final
	{
		Message::Header header;
		std::uint64_t transferID;
		std::uint64_t totalSize;
		std::uint64_t offset;
		std::span<const std::byte> data;
	};

	[[nodiscard]] inline std::shared_ptr<std::vector<std::byte>> encodePage(const Page &page)
	{
		if (page.totalSize != page.message.size() ||
			page.offset > page.totalSize ||
			page.length > page.totalSize - page.offset ||
			page.length > DefaultMaximumPageBytes || page.transferID == 0)
		{
			throw Exception("Invalid outgoing network page.");
		}
		auto result = std::make_shared<std::vector<std::byte>>(
			FrameHeaderSize + PageMetadataSize + page.length);
		auto *bytes = result->data();
		bytes[0] = std::byte{'S'};
		bytes[1] = std::byte{'P'};
		bytes[2] = std::byte{'K'};
		bytes[3] = std::byte{'N'};
		write16(bytes + 4, ProtocolVersion);
		write16(bytes + 6, PageFrameKind);
		write32(bytes + 8, page.message.type());
		write64(bytes + 12, page.message.requestID());
		write64(bytes + 20, page.message.connectionID());
		write32(bytes + 28, static_cast<std::uint32_t>(PageMetadataSize + page.length));
		write64(bytes + FrameHeaderSize, page.transferID);
		write64(bytes + FrameHeaderSize + 8, page.totalSize);
		write64(bytes + FrameHeaderSize + 16, page.offset);
		std::copy(page.data().begin(), page.data().end(), result->begin() + FrameHeaderSize + PageMetadataSize);
		return result;
	}

	[[nodiscard]] inline PageFragment decodePage(std::span<const std::byte> bytes)
	{
		if (bytes.size() < FrameHeaderSize + PageMetadataSize)
		{
			throw Exception("Network page header is incomplete.");
		}
		const auto header = decode(std::span<const std::byte, FrameHeaderSize>(bytes.data(), FrameHeaderSize));
		if (read16(bytes.data() + 6) != PageFrameKind ||
			header.payloadSize < PageMetadataSize ||
			bytes.size() != FrameHeaderSize + header.payloadSize)
		{
			throw Exception("Invalid network page frame.");
		}
		const auto transferID = read64(bytes.data() + FrameHeaderSize);
		const auto totalSize = read64(bytes.data() + FrameHeaderSize + 8);
		const auto offset = read64(bytes.data() + FrameHeaderSize + 16);
		const auto length = header.payloadSize - PageMetadataSize;
		if (transferID == 0)
		{
			throw Exception("Invalid network page offset or transfer ID.");
		}
		return PageFragment{
			Message::Header{header.type, header.requestID, header.connectionID},
			transferID,
			totalSize,
			offset,
			bytes.subspan(FrameHeaderSize + PageMetadataSize, length)};
	}
}
