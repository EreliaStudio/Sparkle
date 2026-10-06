#pragma once

#include <cstdint>

namespace spk::Network
{
	enum class Edit : std::uint8_t
	{
		Set,
		Forget,
		Destroy
	};
}
