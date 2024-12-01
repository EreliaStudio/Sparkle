#include "structure/math/spk_math.hpp"

#include <stdexcept>

namespace spk
{
	float degreeToRadian(float p_degrees)
	{
		return static_cast<float>(p_degrees * (M_PI / 180.0f));
	}

	float radianToDegree(float p_radians)
	{
		return static_cast<float>(p_radians * (180.0f / M_PI));
	}

	intmax_t positiveModulo(const intmax_t& p_value, const intmax_t& p_modulo)
	{
		if (p_modulo == 0)
		{
			throw std::invalid_argument("Modulo by zero is not allowed.");
		}

		intmax_t result = p_value % p_modulo;
		if (result < 0)
		{
			result += p_modulo;
		}
		return result;
	}
}