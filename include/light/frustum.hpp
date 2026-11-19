#pragma once

#include <array>

#include "math/matrix.hpp"
#include "math/vector3.hpp"

namespace spk::Light
{
	class Frustum
	{
	public:
		struct Plane
		{
			Vector3 normal;
			float offset = 0.0f;

			[[nodiscard]] double signedDistance(const Vector3 &position) const noexcept;
		};

	private:
		std::array<Plane, 6> _planes{};

	public:
		// Extracts inward-facing, normalized OpenGL clip-space planes once per camera capture.
		[[nodiscard]] static Frustum fromViewProjection(const Matrix4x4 &viewProjection);

		[[nodiscard]] const std::array<Plane, 6> &planes() const noexcept;
		[[nodiscard]] bool intersectsSphere(const Vector3 &center, float radius) const noexcept;
		// A finite cone, including its apex and circular base.
		[[nodiscard]] bool intersectsCone(
			const Vector3 &apex,
			const Vector3 &unitAxis,
			float length,
			float baseRadius) const noexcept;
	};
}
