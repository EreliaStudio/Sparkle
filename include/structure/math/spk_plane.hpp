#pragma once

#include "structure/math/spk_polygon.hpp"
#include "structure/math/spk_vector3.hpp"
#include <cmath>

namespace spk
{
	struct Plane
	{
		spk::Vector3 origin;
		spk::Vector3 normal;

		Plane() = default;
		Plane(const spk::Vector3 &p_origin, const spk::Vector3 &p_normal) :
			origin(p_origin),
			normal(p_normal.normalize())
		{
		}

		bool contains(const spk::Polygon &p_polygon) const
		{
			const float eps = 1e-5f;
			for (const spk::Vector3 &point : p_polygon.points)
			{
				if (std::abs(normal.dot(point - origin)) > eps)
				{
					return false;
				}
			}
			return true;
		}

		bool contains(const spk::Vector3 &p_point) const
		{
			return (std::abs(normal.dot(p_point - origin)) <= spk::Constants::angularPrecision);
		}

		bool operator == (const spk::Plane &p_plane) const
		{
			if (normal != p_plane.normal && normal != -p_plane.normal)
			{
				return (false);
			}
			return (contains(p_plane.origin));
		}

		bool operator != (const spk::Plane &p_plane) const
		{
			return (!(*this == p_plane));
		}

		bool operator < (const spk::Plane &p_plane) const
		{
			if (normal != p_plane.normal)
			{
				return (normal < p_plane.normal);
			}
			return (origin < p_plane.origin);
		}
	};
}

template<> template<> template<> struct std::hash<spk::Plane>
{
	size_t operator()(const spk::Plane &p_vec) const
	{
		size_t h1 = std::hash<spk::Vector3>()(p_vec.origin);
		size_t h2 = std::hash<spk::Vector3>()(p_vec.normal);
		return h1 ^ (h2 << 1);
	}
};