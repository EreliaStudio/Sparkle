#include "structure/math/spk_edge_map_2d.hpp"

namespace
{
	bool findNextEdge(std::vector<spk::Edge2D> &p_boundary, const spk::Vector2 &p_cursor, spk::Vector2 &p_next)
	{
		for (size_t i = 0; i < p_boundary.size(); ++i)
		{
			const spk::Edge2D &e = p_boundary[i];
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

	std::vector<spk::Edge2D> buildBoundary(
		const std::unordered_map<spk::Edge2D::Identifier, spk::EdgeMap2D::Entry, spk::Edge2D::IdentifierHash> &p_edges)
	{
		std::vector<spk::Edge2D> boundary;
		boundary.reserve(p_edges.size());
		for (const auto &[id, entry] : p_edges)
		{
			if (entry.count == 1)
			{
				boundary.push_back(entry.edge);
			}
		}
		return boundary;
	}

	std::vector<spk::Vector2> traceLoop(std::vector<spk::Edge2D> &p_boundary, const spk::Edge2D &p_start)
	{
		std::vector<spk::Vector2> loop;
		loop.reserve(p_boundary.size() + 1);

		loop.push_back(p_start.first());
		loop.push_back(p_start.second());

		spk::Vector2 cursor = p_start.second();

		while (true)
		{
			spk::Vector2 nextCursor;
			if (findNextEdge(p_boundary, cursor, nextCursor) == false)
			{
				break;
			}

			cursor = nextCursor;
			loop.push_back(cursor);

			if (cursor == loop.front())
			{
				if (loop.back() != loop.front())
				{
					loop.push_back(loop.front());
				}
				return loop;
			}
		}

		if (loop.size() <= 2 || loop.front() != loop.back())
		{
			return {};
		}
		return loop;
	}
}

namespace spk
{
	void EdgeMap2D::addPolygon(const spk::Polygon2D &p_polygon)
	{
		for (const auto &edge : p_polygon.edges())
		{
			spk::Edge2D::Identifier id = spk::Edge2D::Identifier::from(edge);
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

	std::vector<spk::Polygon2D> EdgeMap2D::construct() const
	{
		std::vector<spk::Edge2D> boundary = buildBoundary(_edges);

		std::vector<spk::Polygon2D> polygons;
		polygons.reserve(boundary.size());

		while (boundary.empty() == false)
		{
			spk::Edge2D start = boundary.back();
			boundary.pop_back();

			std::vector<spk::Vector2> loop = traceLoop(boundary, start);
			if (loop.empty() == false)
			{
				loop.pop_back();
				polygons.push_back(spk::Polygon2D::fromLoop(loop));
			}
		}

		return polygons;
	}
}
