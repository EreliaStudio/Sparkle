#pragma once

#include "network/transfer.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace spk::NetworkInternal
{
	struct TransmissionFrame final
	{
		std::shared_ptr<std::vector<std::byte>> bytes;
		spk::Transfer::ID finalPage = spk::Transfer::InvalidID;
	};
}
