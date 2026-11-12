#pragma once

#include <string>

#include "light/contributor.hpp"
#include "light/frustum.hpp"

namespace spk::Light
{
	class Spot final : public Contributor
	{
	private:
		float _range = 10.0f;
		float _angle = 0.5235987756f;
		float _baseRadius = 0.0f;
		float _sphereCenterOffset = 0.0f;
		float _sphereRadius = 0.0f;

		void _updateLocalVolume(float range, float angle);

	public:
		explicit Spot(const std::string &name = "Spot light");

		[[nodiscard]] float range() const noexcept;
		void setRange(float range);
		// Half-angle in radians, strictly between 0 and pi/2.
		[[nodiscard]] float angle() const noexcept;
		void setAngle(float angle);

		[[nodiscard]] Vector3 worldPosition() const;
		// The local forward axis is -Z.
		[[nodiscard]] Vector3 worldDirection() const;
		[[nodiscard]] float baseRadius() const noexcept;
		[[nodiscard]] float boundingSphereRadius() const noexcept;
		[[nodiscard]] Vector3 boundingSphereCenter() const;
		[[nodiscard]] float angularAttenuation(const Vector3 &toReceiver) const;
		[[nodiscard]] float attenuation(float distance, const Vector3 &toReceiver) const;
		[[nodiscard]] bool intersects(const Frustum &frustum) const;
		[[nodiscard]] float priorityScore(
			const Frustum &frustum,
			const Vector3 &cameraPosition,
			float minLightDistance = 0.01f) const;
	};
}
