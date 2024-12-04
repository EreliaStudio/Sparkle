#pragma once

#include "structure/math/spk_vector2.hpp"

namespace spk
{
	struct BoundingBox2D
	{
		spk::Vector2 min;
		spk::Vector2 max;

		void addPoint(const spk::Vector2 &p_point)
		{
			min = spk::Vector2::min(min, p_point);
			max = spk::Vector2::max(max, p_point);
		}

		BoundingBox2D place(const spk::Vector2 &p_delta) const
		{
			BoundingBox2D result;

			result.min = min + p_delta;
			result.max = max + p_delta;

			return (result);
		}

		bool intersect(const BoundingBox2D &p_other) const
		{
			return ((min.x <= p_other.max.x && max.x >= p_other.min.x && min.y <= p_other.max.y && max.y >= p_other.min.y) == true);
		}

		bool contains(const spk::Vector2 &p_point) const
		{
			return ((p_point.x >= min.x && p_point.x <= max.x && p_point.y >= min.y && p_point.y <= max.y) == true);
		}
	};
}
