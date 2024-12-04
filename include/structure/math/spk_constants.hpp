#pragma once

#include <cmath>

namespace spk
{
	namespace Constants
	{
		inline constexpr float pointPrecision = 1e-5f;
		inline constexpr float angularPrecision = 1e-5f;
		inline constexpr float colorPrecision = 1e-4f;
	}
}

#define FLOAT_EQ(a, b) (std::fabs((a) - (b)) < spk::Constants::pointPrecision)
#define FLOAT_NEQ(a, b) (std::fabs((a) - (b)) >= spk::Constants::pointPrecision)
