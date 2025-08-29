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

		bool intersect(const BoundingBox2D &p_other) const
		{
			return ((min.x <= p_other.max.x && max.x >= p_other.min.x && min.y <= p_other.max.y && max.y >= p_other.min.y) == true);
		}
	};
}
