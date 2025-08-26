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
			for (const spk::Vector3 &point : p_polygon.points())
			{
				if (std::abs(normal.dot(point - origin)) > eps)
				{
					return false;
				}
			}
			return true;
		}
	};
}
