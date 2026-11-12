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
			const int coordinate = coordinates[i];
			const double sign = signs[i];
			const double x = static_cast<double>(m[0][3]) + sign * m[0][coordinate];
			const double y = static_cast<double>(m[1][3]) + sign * m[1][coordinate];
			const double z = static_cast<double>(m[2][3]) + sign * m[2][coordinate];
			const double d = static_cast<double>(m[3][3]) + sign * m[3][coordinate];
			const double length = std::hypot(std::hypot(x, y), z);

			if (!std::isfinite(length) || length <= 0.0 || !std::isfinite(d))
			{
				throw spk::Exception("Cannot extract normalized planes from the camera matrix");
			}

			const double nx = x / length;
			const double ny = y / length;
			const double nz = z / length;
			const double nd = d / length;
			if (!std::isfinite(nd) || std::abs(nd) > std::numeric_limits<float>::max())
			{
				throw spk::Exception("Camera frustum plane is outside the representable range");
			}

			result._planes[i] = {{static_cast<float>(nx), static_cast<float>(ny), static_cast<float>(nz)}, static_cast<float>(nd)};
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
			const double support = std::max(0.0, static_cast<double>(length) * projection +
													   static_cast<double>(baseRadius) * perpendicular);
			if (plane.signedDistance(apex) + support < 0.0)
			{
				return false;
			}
		}
		return true;
	}
}
