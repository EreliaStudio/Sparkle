#pragma once

#include "structure/math/spk_bounding_box_2d.hpp"
#include "structure/math/spk_edge_2d.hpp"
#include "structure/math/spk_vector2.hpp"

#include <vector>

namespace spk
{
	class Polygon2D
	{
	private:
		std::vector<spk::Vector2> _points;
		BoundingBox2D _boundingBox;

	public:
		Polygon2D() = default;
		Polygon2D(const std::vector<spk::Vector2> &p_points) :
			_points(p_points)
		{
			for (const auto &p : _points)
			{
				_boundingBox.addPoint(p);
			}
		}

		const std::vector<spk::Vector2> &points() const
		{
			return _points;
		}

		const BoundingBox2D &boundingBox() const
		{
			return _boundingBox;
		}

		static Polygon2D fromLoop(const std::vector<spk::Vector2> &p_points)
		{
			return Polygon2D(p_points);
		}
	};
}
