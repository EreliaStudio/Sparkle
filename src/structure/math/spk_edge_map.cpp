#include "structure/math/spk_edge_map.hpp"
#include <iostream>

namespace
{
	bool findNextEdge(std::vector<spk::Edge> &p_boundary, const spk::Vector3 &p_cursor, spk::Vector3 &p_next)
	{
		for (size_t i = 0; i < p_boundary.size(); ++i)
		{
			const spk::Edge &e = p_boundary[i];
			if (e.first() == p_cursor)
			{
				p_next = e.second();
				p_boundary.erase(p_boundary.begin() + i);
				return true;
			}
			if (e.second() == p_cursor)
			{
				p_next = e.first();
				p_boundary.erase(p_boundary.begin() + i);
				return true;
			}
		}
		return false;
	}
}

namespace spk
{
	void EdgeMap::addPolygon(const spk::Polygon &p_polygon)
	{
		for (const auto &edge : p_polygon.edges())
		{
			spk::Edge::Identifier id = spk::Edge::Identifier::from(edge);
			auto it = _edges.find(id);
			if (it == _edges.end())
			{
				_edges.emplace(id, Entry{edge, 1});
			}
			else
			{
				it->second.count += 1;
			}
		}
	}

	std::vector<spk::Polygon> EdgeMap::construct() const
	{
		std::vector<spk::Polygon> polygons;

		std::vector<spk::Edge> boundary;
		boundary.reserve(_edges.size());
		for (const auto &[id, entry] : _edges)
		{
			if (entry.count == 1)
			{
				boundary.push_back(entry.edge);
			}
		}

		while (boundary.empty() == false)
		{
			std::vector<spk::Vector3> loop;
			loop.reserve(boundary.size() + 1);

			spk::Edge current = boundary.back();
			boundary.pop_back();
			loop.push_back(current.first());
			loop.push_back(current.second());
			spk::Vector3 cursor = current.second();

			bool loopClosed = false;

			while (boundary.empty() == false)
			{
				spk::Vector3 nextCursor;
				if (findNextEdge(boundary, cursor, nextCursor) == false)
				{
					break;
				}
				cursor = nextCursor;
				loop.push_back(cursor);

				if (cursor == loop.front())
				{
					loopClosed = true;
					break;
				}
			}

			if (loopClosed == true && loop.size() > 2)
			{
				if (loop.back() != loop.front())
				{
					loop.push_back(loop.front());
				}
				polygons.push_back(spk::Polygon::fromLoop(loop));
			}
		}

		return polygons;
	}
}