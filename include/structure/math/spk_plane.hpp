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
			return (std::abs(normal.dot(p_point - origin)) <= spk::Constants::pointPrecision);
		}

		bool operator==(const spk::Plane &p_plane) const
		{
			spk::Vector3 n = normal.normalize();
			spk::Vector3 otherN = p_plane.normal.normalize();

			if ((n != otherN) == true && (n != -otherN) == true)
			{
				return (false);
			}
			return ((origin == p_plane.origin) == true);
		}

		bool operator!=(const spk::Plane &p_plane) const
		{
			return (!(*this == p_plane));
		}

		bool operator<(const spk::Plane &p_plane) const
		{
			if (normal != p_plane.normal)
			{
				return (normal < p_plane.normal);
			}
			return (origin < p_plane.origin);
		}
	};
}

namespace std
{
	template <>
	struct hash<spk::Plane>
	{
		size_t operator()(const spk::Plane &p_plane) const
		{
			spk::Vector3 n = p_plane.normal.normalize();

			if ((-n < n) == true)
			{
				n = n * -1.0f;
			}

			size_t h = 0;
			h ^= std::hash<spk::Vector3>()(n) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<spk::Vector3>()(p_plane.origin) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return (h);
		}
	};
}
