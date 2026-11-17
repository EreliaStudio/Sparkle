#pragma once

#include <cstdint>

namespace spk::Light
{
	// One nearest depth comparison or a 3x3 PCF footprint with bilinear depth comparisons.
	enum class ShadowFilter : std::uint8_t
	{
		Hard,
		PCF3x3
	};
}
