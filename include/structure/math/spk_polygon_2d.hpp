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
		Polygon2D(const std::vector<spk::Vector2> &p_points);

		const std::vector<spk::Vector2> &points() const;

		const BoundingBox2D &boundingBox() const;

		static Polygon2D fromLoop(const std::vector<spk::Vector2> &p_points);

		bool intersect(const spk::Polygon2D& p_other) const;
	};
}
