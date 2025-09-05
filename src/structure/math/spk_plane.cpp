#include "structure/math/spk_plane.hpp"
#include "structure/math/spk_constants.hpp"
#include "structure/math/spk_polygon.hpp"

#include <cmath>

namespace spk
{
	Plane::Identifier Plane::Identifier::from(const spk::Plane &p_plane)
	{
		spk::Vector3 normal = p_plane.normal.normalize();
		float dotValue = normal.dot(p_plane.origin);

		if ((-normal < normal) == true)
		{
			normal = -normal;
		}

		return {normal, dotValue};
	}

	bool Plane::Identifier::operator==(const Plane::Identifier &p_other) const
	{
		return (normal == p_other.normal && FLOAT_EQ(dotValue, p_other.dotValue));
	}

	Plane::Plane(const spk::Vector3 &p_normal, const spk::Vector3 &p_origin) :
		origin(p_origin),
		normal(p_normal.normalize())
	{
	}

	bool Plane::contains(const spk::Polygon &p_polygon) const
	{
		const float eps = 1e-5f;
		for (const auto &edge : p_polygon.edges())
		{
			if (std::abs(normal.dot(edge.first() - origin)) > eps || std::abs(normal.dot(edge.second() - origin)) > eps)
			{
				return false;
			}
		}
		return true;
	}

	bool Plane::contains(const spk::Vector3 &p_point) const
	{
		return (std::abs(normal.dot(p_point - origin)) <= spk::Constants::pointPrecision);
	}

	bool Plane::isSame(const spk::Plane &p_plane) const
	{
		if (normal != p_plane.normal && normal != p_plane.normal.inverse())
		{
			return (false);
		}
		return (contains(p_plane.origin));
	}

	bool Plane::operator==(const spk::Plane &p_plane) const
	{
		spk::Vector3 n = normal.normalize();
		spk::Vector3 otherN = p_plane.normal.normalize();

		if (n != otherN && n != -otherN)
		{
			return false;
		}
		return (origin == p_plane.origin);
	}

	bool Plane::operator!=(const spk::Plane &p_plane) const
	{
		return !(*this == p_plane);
	}

	bool Plane::operator<(const spk::Plane &p_plane) const
	{
		if (normal != p_plane.normal)
		{
			return (normal < p_plane.normal);
		}
		return (origin < p_plane.origin);
	}
}
