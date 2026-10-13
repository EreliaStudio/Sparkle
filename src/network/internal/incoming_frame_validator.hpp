#pragma once

#include "exception.hpp"
#include "network/internal/frame.hpp"
#include "network/internal/transfer_response.hpp"

#include <cstddef>
#include <cstdint>

namespace spk::NetworkInternal
{
	class IncomingFrameValidator final
	{
	public:
		static void validate(std::uint16_t kind, std::size_t bytes, std::size_t maximumPageBytes, std::size_t maximumMessageBytes)
		{
			if (kind == 0 && bytes <= maximumMessageBytes)
			{
				return;
			}
			if (kind == PageFrameKind && bytes >= PageMetadataSize &&
				bytes - PageMetadataSize <= maximumPageBytes)
			{
				return;
			}
			if (kind == TransferResponse::FrameKind && bytes == TransferResponse::PayloadSize)
			{
				return;
			}
			throw Exception("Invalid incoming transport frame kind or payload length.");
		}
	};
}
