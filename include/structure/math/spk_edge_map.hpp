#pragma once

#include "structure/math/spk_edge.hpp"
#include "structure/math/spk_polygon.hpp"

#include <unordered_map>
#include <vector>

namespace spk
{
	class EdgeMap
	{
	private:
		struct Entry
		{
			spk::Edge edge;
			size_t count = 0;
		};

		std::unordered_map<spk::Edge::Identifier, Entry, spk::Edge::IdentifierHash> _edges;

	public:
		void addPolygon(const spk::Polygon &p_polygon);
		spk::Polygon construct() const;
	};
}