#include "light/spot.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include "engine/entity3d.hpp"
#include "engine/light/local_light_math.hpp"
#include "exception.hpp"

namespace spk::Light
{
	Spot::Spot(const std::string &name) :
		Contributor(name, true)
	{
		_updateLocalVolume(_range, _angle);
	}

	void Spot::_updateLocalVolume(float range, float angle)
	{
		if (!std::isfinite(range) || range < 0.0f)
		{
			throw spk::Exception("Spot light range must be finite and nonnegative");
		}
		if (!std::isfinite(angle) || angle <= 0.0f ||
			static_cast<double>(angle) >= std::numbers::pi_v<double> / 2.0 ||
			std::cos(static_cast<double>(angle)) >= 1.0)
		{
			throw spk::Exception("Spot light angle must be nondegenerate and in (0, pi/2) radians");
		}

		const double length = range;
		const double baseRadius = length * std::tan(static_cast<double>(angle));
		const double centerOffset = (range == 0.0f)
										? 0.0
										: std::min(length, (length * length + baseRadius * baseRadius) / (2.0 * length));
		const double sphereRadius = std::max(centerOffset, std::hypot(length - centerOffset, baseRadius));
		const double limit = std::numeric_limits<float>::max();
		if (!std::isfinite(baseRadius) || !std::isfinite(centerOffset) || !std::isfinite(sphereRadius) ||
			baseRadius > limit || centerOffset > limit || sphereRadius > limit)
		{
			throw spk::Exception("Spot influence geometry is not representable");
		}

		_range = range;
		_angle = angle;
		_baseRadius = static_cast<float>(baseRadius);
		_sphereCenterOffset = static_cast<float>(centerOffset);
		_sphereRadius = static_cast<float>(sphereRadius);
	}

	float Spot::range() const noexcept
	{
		return _range;
	}

	void Spot::setRange(float range)
	{
		if (_range == range)
		{
			return;
		}
		_updateLocalVolume(range, _angle);
		_notifyEdition();
	}

	float Spot::angle() const noexcept
	{
		return _angle;
	}

	void Spot::setAngle(float angle)
	{
		if (_angle == angle)
		{
			return;
		}
		_updateLocalVolume(_range, angle);
		_notifyEdition();
	}

	Vector3 Spot::worldPosition() const
	{
		return _worldPosition();
	}

	Vector3 Spot::worldDirection() const
	{
		return _worldForward();
	}

	float Spot::baseRadius() const noexcept
	{
		return _baseRadius;
	}

	float Spot::boundingSphereRadius() const noexcept
	{
		return _sphereRadius;
	}

	Vector3 Spot::boundingSphereCenter() const
	{
		const Vector3 apex = worldPosition();
		const Vector3 axis = worldDirection();
		return apex + Vector3{axis.x * _sphereCenterOffset, axis.y * _sphereCenterOffset, axis.z * _sphereCenterOffset};
	}

	float Spot::angularAttenuation(const Vector3 &toReceiver) const
	{
		const double norm = toReceiver.length();
		if (!std::isfinite(norm))
		{
			throw spk::Exception("Spot receiver direction must be finite");
		}
		if (norm == 0.0)
		{
			return 1.0f;
		}
		const Vector3 axis = worldDirection();
		const double cosToReceiver = (static_cast<double>(axis.x) * toReceiver.x + static_cast<double>(axis.y) * toReceiver.y + static_cast<double>(axis.z) * toReceiver.z) / norm;
		const double cosEdge = std::cos(static_cast<double>(_angle));
		const double t = std::clamp((cosToReceiver - cosEdge) / (1.0 - cosEdge), 0.0, 1.0);
		return static_cast<float>(3.0 * t * t - 2.0 * t * t * t);
	}

	float Spot::attenuation(float distance, const Vector3 &toReceiver) const
	{
		const float radial = detail::radialAttenuation(distance, _range);
		return radial == 0.0f ? 0.0f : radial * angularAttenuation(toReceiver);
	}

	bool Spot::intersects(const Frustum &frustum) const
	{
		if (_range == 0.0f || owner() == nullptr)
		{
			return false;
		}

		const Vector3 apex = worldPosition();
		const Vector3 axis = worldDirection();
		const Vector3 center = apex + Vector3{axis.x * _sphereCenterOffset, axis.y * _sphereCenterOffset, axis.z * _sphereCenterOffset};
		return frustum.intersectsSphere(center, _sphereRadius) &&
			   frustum.intersectsCone(apex, axis, _range, _baseRadius);
	}

	float Spot::priorityScore(
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
