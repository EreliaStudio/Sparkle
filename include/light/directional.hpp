#pragma once

#include <string>

#include "light/contributor.hpp"

namespace spk::Light
{
	class Directional final : public Contributor
	{
	private:
		// U: shadow coverage only; illumination remains unbounded. PR17 default: 100 U.
		float _shadowDistance = 100.0f;

	public:
		explicit Directional(const std::string &name = "Directional light");

		[[nodiscard]] Vector3 worldDirection() const;
		[[nodiscard]] float shadowDistance() const noexcept;
		void setShadowDistance(float distance);
		// Directional illumination has no radial or angular attenuation.
		[[nodiscard]] float attenuation() const noexcept;
	};
}
