#pragma once

#include "structure/math/spk_edge_2d.hpp"
#include "structure/math/spk_polygon_2d.hpp"

#include <unordered_map>
#include <vector>

namespace spk
{
	class EdgeMap2D
	{
	private:
		struct Entry
		{
			spk::Edge2D edge;
			size_t count = 0;
		};

		std::unordered_map<spk::Edge2D::Identifier, Entry, spk::Edge2D::IdentifierHash> _edges;

	public:
		void addPolygon(const spk::Polygon2D &p_polygon)
		{
			const auto &pts = p_polygon.points();
			for (size_t i = 0; i < pts.size(); ++i)
			{
				spk::Edge2D e(pts[i], pts[(i + 1) % pts.size()]);
				auto id = spk::Edge2D::Identifier::from(e);
				auto &entry = _edges[id];
				entry.edge = e;
				entry.count++;
			}
		}

		std::vector<spk::Polygon2D> construct() const
		{
			std::vector<spk::Polygon2D> result;
			for (const auto &pair : _edges)
			{
				if (pair.second.count == 1)
				{
					result.push_back(spk::Polygon2D::fromLoop({pair.second.edge.first(), pair.second.edge.second()}));
				}
			}
			return result;
		}
	};
}
