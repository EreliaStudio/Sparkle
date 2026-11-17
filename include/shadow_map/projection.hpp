#pragma once

#include <array>

#include "light/selection.hpp"
#include "math/camera_3d.hpp"
#include "math/matrix.hpp"

namespace spk::ShadowMap
{
	struct Projection
	{
		Matrix4x4 viewProjection = Matrix4x4::identity();
		float nearPlane = 0.0f;
		float farPlane = 0.0f;

		[[nodiscard]] static std::array<Projection, 6> point(const Light::Selection::Entry &light);
		[[nodiscard]] static Projection spot(const Light::Selection::Entry &light);
		[[nodiscard]] static Projection directional(const Light::Selection::Entry &light, const Camera3D &camera);
	};
}
