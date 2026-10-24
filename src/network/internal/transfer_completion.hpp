#pragma once

#include "network/transfer.hpp"

namespace spk::NetworkInternal
{
	class TransferCompletion final
	{
	public:
		static void assignID(spk::Transfer &transfer, spk::Transfer::ID id) noexcept
		{
			transfer._assignID(id);
		}

		[[nodiscard]] static bool complete(
			spk::Transfer &transfer,
			spk::Transfer::Status status,
			spk::Transfer::Reason reason = spk::Transfer::Reason::None) noexcept
		{
			return transfer._complete(status, reason);
		}
	};
}
