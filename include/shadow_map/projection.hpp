#pragma once

#include <array>
#include <span>

#include "light/selection.hpp"
#include "math/camera_3d.hpp"
#include "math/matrix.hpp"

namespace spk::ShadowMap
{
	struct CasterBounds
	{
		Vector3 minimum;
		Vector3 maximum;
	};

	struct Projection
	{
		Matrix4x4 viewProjection = Matrix4x4::identity();
		float nearPlane = 0.0f;
		float farPlane = 0.0f;
		float splitFar = 0.0f;
		float worldPerTexel = 0.0f;
		float splitNear = 0.0f;
		static constexpr std::size_t CascadeCount = 4;

		[[nodiscard]] static std::array<Projection, 6> point(const Light::Selection::Entry &light);
		[[nodiscard]] static Projection spot(const Light::Selection::Entry &light);
		[[nodiscard]] static Projection directional(const Light::Selection::Entry &light, const Camera3D &camera);
		[[nodiscard]] static std::array<Projection, CascadeCount> directionalCascades(
			const Light::Selection::Entry &light, const Camera3D &camera, std::span<const CasterBounds> casters,
			std::size_t count = CascadeCount);
		[[nodiscard]] bool intersects(const CasterBounds &bounds) const;
	};
}
