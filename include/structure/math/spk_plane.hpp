#pragma once

#include "structure/math/spk_vector3.hpp"
#include <cmath>

namespace spk
{
	struct Polygon;

	struct Plane
	{
		spk::Vector3 origin;
		spk::Vector3 normal;

		Plane() = default;
		Plane(const spk::Vector3 &p_normal, const spk::Vector3 &p_origin);
		bool contains(const spk::Polygon &p_polygon) const;
		bool contains(const spk::Vector3 &p_point) const;
		bool operator==(const spk::Plane &p_plane) const;
		bool operator!=(const spk::Plane &p_plane) const;
		bool operator<(const spk::Plane &p_plane) const;
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
