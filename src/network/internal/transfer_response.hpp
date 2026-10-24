#pragma once

#include "network/connection.hpp"
#include "network/transfer.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace spk::NetworkInternal
{
	struct TransferResponse
	{
		struct Codec
		{
			inline static constexpr std::uint16_t FrameKind = 2;
			inline static constexpr std::size_t PayloadSize = 16;
			[[nodiscard]] static std::shared_ptr<std::vector<std::byte>> encode(const TransferResponse &response);
			[[nodiscard]] static TransferResponse decode(std::span<const std::byte> bytes);
		};

		Connection::ID connectionID;
		spk::Transfer::ID transferID;
		spk::Transfer::Status status;
		spk::Transfer::Reason reason;

		[[nodiscard]] bool valid() const noexcept;
	};
}
