#pragma once

#include "structure/math/spk_edge_2d.hpp"
#include "structure/math/spk_polygon_2d.hpp"

#include <unordered_map>
#include <vector>

namespace spk
{
	class EdgeMap2D
	{
	public:
		struct Entry
		{
			spk::Edge2D edge;
			size_t count = 0;
		};

	private:
		std::unordered_map<spk::Edge2D::Identifier, Entry, spk::Edge2D::IdentifierHash> _edges;

	public:
		void addPolygon(const spk::Polygon2D &p_polygon);
		std::vector<spk::Polygon2D> construct() const;
	};
}
