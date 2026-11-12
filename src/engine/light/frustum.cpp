#include "light/frustum.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "exception.hpp"

namespace spk::Light
{
	double Frustum::Plane::signedDistance(const Vector3 &position) const noexcept
	{
		return static_cast<double>(normal.x) * position.x +
			   static_cast<double>(normal.y) * position.y +
			   static_cast<double>(normal.z) * position.z + offset;
	}

	Frustum Frustum::fromViewProjection(const Matrix4x4 &m)
	{
		Frustum result;
		constexpr int coordinates[6] = {0, 0, 1, 1, 2, 2};
		constexpr double signs[6] = {1, -1, 1, -1, 1, -1};

		for (std::size_t i = 0; i < result._planes.size(); ++i)
		{
			// Promote before arithmetic: summing large float rows must not overflow.
			const TVector4<double> coefficients =
				TVector4<double>(m.row(3)) + TVector4<double>(m.row(coordinates[i])) * signs[i];
			const double length = coefficients.xyz().length();

			if (!std::isfinite(length) || length <= 0.0 || !std::isfinite(coefficients.w))
			{
				throw spk::Exception("Cannot extract normalized planes from the camera matrix");
			}

			const TVector3<double> normal = coefficients.xyz() / TVector3<double>{length, length, length};
			const double offset = coefficients.w / length;
			if (!std::isfinite(offset) || std::abs(offset) > std::numeric_limits<float>::max())
			{
				throw spk::Exception("Camera frustum plane is outside the representable range");
			}

			result._planes[i] = {Vector3(normal), static_cast<float>(offset)};
		}
		return result;
	}

	const std::array<Frustum::Plane, 6> &Frustum::planes() const noexcept
	{
		return _planes;
	}

	bool Frustum::intersectsSphere(const Vector3 &center, float radius) const noexcept
	{
		for (const Plane &plane : _planes)
		{
			if (plane.signedDistance(center) < -static_cast<double>(radius))
			{
				return false;
			}
		}
		return true;
	}

	bool Frustum::intersectsCone(
		const Vector3 &apex,
		const Vector3 &unitAxis,
		float length,
		float baseRadius) const noexcept
	{
		for (const Plane &plane : _planes)
		{
			const double projection = static_cast<double>(plane.normal.x) * unitAxis.x +
									  static_cast<double>(plane.normal.y) * unitAxis.y +
									  static_cast<double>(plane.normal.z) * unitAxis.z;
			const double perpendicular = std::sqrt(std::max(0.0, 1.0 - projection * projection));
			const double support = std::max(0.0, static_cast<double>(length) * projection + static_cast<double>(baseRadius) * perpendicular);
			if (plane.signedDistance(apex) + support < 0.0)
			{
				return false;
			}
		}
		return true;
	}
}
