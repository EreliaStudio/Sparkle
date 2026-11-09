#pragma once

#include <cstdint>

namespace spk
{
	enum class TextureCubeFace : std::uint8_t
	{
		PositiveX,
		NegativeX,
		PositiveY,
		NegativeY,
		PositiveZ,
		NegativeZ
	};
}
