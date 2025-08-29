#pragma once

#include "structure/math/spk_vector3.hpp"

namespace spk
{
	struct BoundingBox
	{
		spk::Vector3 min;
		spk::Vector3 max;

		void addPoint(const spk::Vector3& p_point)
		{
			min = spk::Vector3::min(min, p_point);
			max = spk::Vector3::max(max, p_point);
		}

		bool intersect(const BoundingBox& p_other) const
		{
			return (min.x <= p_other.max.x && max.x >= p_other.min.x) &&
				   (min.y <= p_other.max.y && max.y >= p_other.min.y) &&
				   (min.z <= p_other.max.z && max.z >= p_other.min.z);
		}
	};
}