#pragma once

#include "network/internal/command_budget.hpp"
#include "network/transfer.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace spk::NetworkInternal
{
	struct TransmissionFrame final
	{
		std::shared_ptr<std::vector<std::byte>> bytes;
		spk::Transfer::ID finalPage = spk::Transfer::InvalidID;
		std::uint16_t handshakeKind = 0;
		std::shared_ptr<CommandBudget::Lease> reservation;
		spk::Transfer::ID transferID = spk::Transfer::InvalidID;
	};
}
