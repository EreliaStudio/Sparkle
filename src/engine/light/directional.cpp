#include "light/directional.hpp"

#include <cmath>

#include "exception.hpp"

namespace spk::Light
{
	Directional::Directional(const std::string &name) :
		Contributor(name, true)
	{
	}

	Vector3 Directional::worldDirection() const
	{
		return _worldForward();
	}

	float Directional::shadowDistance() const noexcept
	{
		return _shadowDistance;
	}

	void Directional::setShadowDistance(float distance)
	{
		if (!std::isfinite(distance) || distance < 0.0f)
		{
			throw spk::Exception("Directional shadowDistance must be finite and nonnegative");
		}
		if (_shadowDistance == distance)
		{
			return;
		}
		_shadowDistance = distance;
		_notifyEdition();
	}

	float Directional::attenuation() const noexcept
	{
		return 1.0f;
	}
}
