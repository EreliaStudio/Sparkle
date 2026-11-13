#pragma once

#include <string>

#include "light/contributor.hpp"
#include "light/frustum.hpp"

namespace spk::Light
{
	class Point final : public Contributor
	{
	private:
		float _range = 10.0f;

	public:
		explicit Point(const std::string &name = "Point light");

		[[nodiscard]] float range() const noexcept;
		void setRange(float range);
		[[nodiscard]] Vector3 worldPosition() const;
		[[nodiscard]] float attenuation(float distance) const;
		[[nodiscard]] bool intersects(const Frustum &frustum) const;
		[[nodiscard]] float priorityScore(
			const Frustum &frustum,
			const Vector3 &cameraPosition,
			float minLightDistance = 0.01f) const;
	};
}
