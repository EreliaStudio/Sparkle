#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <stdint.h>

namespace spk
{
	float degreeToRadian(float p_degrees);
	float radianToDegree(float p_radians);

	intmax_t positiveModulo(const intmax_t& p_value, const intmax_t& p_modulo);

}