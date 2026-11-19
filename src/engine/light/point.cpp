#include "light/point.hpp"

#include <cmath>

#include "engine/light/local_light_math.hpp"
#include "exception.hpp"

namespace spk::Light
{
	Point::Point(const std::string &name) :
		Contributor(name, false)
	{
	}

	float Point::range() const noexcept
	{
		return _range;
	}

	void Point::setRange(float range)
	{
		if (!std::isfinite(range) || range < 0.0f)
		{
			throw spk::Exception("Point light range must be finite and nonnegative");
		}
		if (_range == range)
		{
			return;
		}
		_range = range;
		_notifyEdition();
	}

	Vector3 Point::worldPosition() const
	{
		return _worldPosition();
	}

	float Point::attenuation(float distance) const
	{
		return detail::radialAttenuation(distance, _range);
	}

	bool Point::intersects(const Frustum &frustum) const
	{
		return _range > 0.0f && owner() != nullptr &&
			   frustum.intersectsSphere(worldPosition(), _range);
	}

	float Point::priorityScore(
		const Frustum &frustum,
		const Vector3 &cameraPosition,
		float minLightDistance) const
	{
		if (!std::isfinite(minLightDistance) || minLightDistance <= 0.0f)
		{
			throw spk::Exception("minLightDistance must be finite and strictly positive");
		}
		if (!isEffectivelyActive() || _range == 0.0f || intensity() == 0.0f ||
			priorityWeight() == 0.0f || !intersects(frustum))
		{
			return 0.0f;
		}
		return detail::localPriorityScore(intensity(), _range, priorityWeight(), worldPosition(), cameraPosition, minLightDistance);
	}
}
