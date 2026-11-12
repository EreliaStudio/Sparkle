#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "exception.hpp"
#include "math/vector3.hpp"

namespace spk::Light::detail
{
	[[nodiscard]] inline float radialAttenuation(float distance, float range)
	{
		if (!std::isfinite(distance) || distance < 0.0f)
		{
			throw spk::Exception("Direct light attenuation distance must be finite and nonnegative");
		}
		if (range == 0.0f || distance >= range)
		{
			return 0.0f;
		}
		const double x = std::clamp(static_cast<double>(distance) / range, 0.0, 1.0);
		return static_cast<float>(1.0 - 3.0 * x * x + 2.0 * x * x * x);
	}

	[[nodiscard]] inline float localPriorityScore(
		float intensity,
		float range,
		float weight,
		const Vector3 &sourcePosition,
		const Vector3 &cameraPosition,
		float minLightDistance)
	{
		if (!std::isfinite(minLightDistance) || minLightDistance <= 0.0f)
		{
			throw spk::Exception("minLightDistance must be finite and strictly positive");
		}
		if (!std::isfinite(cameraPosition.x) || !std::isfinite(cameraPosition.y) ||
			!std::isfinite(cameraPosition.z))
		{
			throw spk::Exception("Camera world position must be finite");
		}
		if (range == 0.0f || intensity == 0.0f || weight == 0.0f)
		{
			return 0.0f;
		}

		const double dx = static_cast<double>(sourcePosition.x) - cameraPosition.x;
		const double dy = static_cast<double>(sourcePosition.y) - cameraPosition.y;
		const double dz = static_cast<double>(sourcePosition.z) - cameraPosition.z;
		const double distance = std::hypot(std::hypot(dx, dy), dz);
		const double result = static_cast<double>(intensity) * range * range * weight /
							  std::max(distance, static_cast<double>(minLightDistance));
		if (!std::isfinite(result) || result > std::numeric_limits<float>::max())
		{
			throw spk::Exception("Direct light priority score cannot be represented as a finite float");
		}
		return static_cast<float>(result);
	}
}
