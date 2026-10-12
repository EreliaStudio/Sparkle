#pragma once

#include "exception.hpp"
#include "network/internal/frame.hpp"
#include "network/transfer.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace spk::NetworkInternal
{
	class TransferResponse final
	{
	public:
		inline static constexpr std::uint16_t FrameKind = 2;
		inline static constexpr std::size_t PayloadSize = 16;

		Connection::ID connectionID;
		spk::Transfer::ID transferID;
		spk::Transfer::Status status;
		spk::Transfer::Reason reason;

		[[nodiscard]] bool valid() const noexcept
		{
			if (connectionID == Connection::InvalidID || transferID == spk::Transfer::InvalidID)
			{
				return false;
			}
			if (status != spk::Transfer::Status::Success &&
				status != spk::Transfer::Status::Rejected &&
				status != spk::Transfer::Status::Failed)
			{
				return false;
			}
			if (reason > spk::Transfer::Reason::TransportFailure)
			{
				return false;
			}
			if (status == spk::Transfer::Status::Success)
			{
				return reason == spk::Transfer::Reason::None;
			}
			return reason != spk::Transfer::Reason::None;
		}

		[[nodiscard]] std::shared_ptr<std::vector<std::byte>> encode() const
		{
			if (!valid())
			{
				throw Exception("Invalid network transfer response.");
			}
			auto frame = std::make_shared<std::vector<std::byte>>(FrameHeaderSize + PayloadSize);
			auto *data = frame->data();
			data[0] = std::byte{'S'};
			data[1] = std::byte{'P'};
			data[2] = std::byte{'K'};
			data[3] = std::byte{'N'};
			write16(data + 4, ProtocolVersion);
			write16(data + 6, FrameKind);
			write32(data + 8, 0);
			write64(data + 12, 0);
			write64(data + 20, connectionID);
			write32(data + 28, PayloadSize);
			write64(data + FrameHeaderSize, transferID);
			write16(data + FrameHeaderSize + 8, static_cast<std::uint16_t>(status));
			write16(data + FrameHeaderSize + 10, static_cast<std::uint16_t>(reason));
			write32(data + FrameHeaderSize + 12, 0);
			return frame;
		}

		[[nodiscard]] static TransferResponse decode(std::span<const std::byte> bytes)
		{
			if (bytes.size() != FrameHeaderSize + PayloadSize)
			{
				throw Exception("Invalid network transfer response length.");
			}
			const auto header = NetworkInternal::decode(
				std::span<const std::byte, FrameHeaderSize>(bytes.data(), FrameHeaderSize));
			if (read16(bytes.data() + 6) != FrameKind || header.payloadSize != PayloadSize ||
				header.type != 0 || header.requestID != 0 ||
				read32(bytes.data() + FrameHeaderSize + 12) != 0)
			{
				throw Exception("Invalid network transfer response frame.");
			}
			const TransferResponse response{
				header.connectionID,
				read64(bytes.data() + FrameHeaderSize),
				static_cast<spk::Transfer::Status>(read16(bytes.data() + FrameHeaderSize + 8)),
				static_cast<spk::Transfer::Reason>(read16(bytes.data() + FrameHeaderSize + 10))};
			if (!response.valid())
			{
				throw Exception("Invalid network transfer response metadata.");
			}
			return response;
		}
	};
}
