#pragma once

#include <cstdint>

namespace spk::Network
{
	enum class Failure : std::uint8_t
	{
		Transient,
		Permanent
	};
}
