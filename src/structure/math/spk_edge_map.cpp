#include "structure/math/spk_edge_map.hpp"
#include <iostream>

namespace
{
	struct BoundaryEdge
	{
		spk::Edge edge;
		spk::Plane plane;
	};

	bool findNextEdge(std::vector<BoundaryEdge> &p_boundary, const spk::Vector3 &p_cursor, spk::Vector3 &p_next, const spk::Plane &p_plane)
	{
		for (size_t i = 0; i < p_boundary.size(); ++i)
		{
			const BoundaryEdge &be = p_boundary[i];
			if (be.plane.isSame(p_plane) == false)
			{
				continue;
			}
			const spk::Edge &e = be.edge;
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

	static std::vector<BoundaryEdge> buildBoundary(const std::unordered_map<spk::Edge::Identifier, spk::EdgeMap::Entry, spk::Edge::IdentifierHash> &p_edges)
	{
		std::vector<BoundaryEdge> boundary;
		boundary.reserve(p_edges.size());
		for (const auto &[id, entry] : p_edges)
		{
			if (entry.count == 1)
			{
				boundary.push_back({entry.edge, entry.plane});
			}
		}
		return boundary;
	}

	static std::vector<spk::Vector3> traceLoop(std::vector<BoundaryEdge> &p_boundary, const BoundaryEdge &p_start)
	{
		std::vector<spk::Vector3> loop;
		loop.reserve(p_boundary.size() + 1);

		loop.push_back(p_start.edge.first());
		loop.push_back(p_start.edge.second());

		spk::Plane plane = p_start.plane;
		spk::Vector3 cursor = p_start.edge.second();

		while (true)
		{
			spk::Vector3 nextCursor;
			if (findNextEdge(p_boundary, cursor, nextCursor, plane) == false)
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

	static bool isContainedByAny(const std::vector<spk::Polygon> &p_polygons, size_t p_index)
	{
		const spk::Polygon &poly = p_polygons[p_index];
		for (size_t j = 0; j < p_polygons.size(); ++j)
		{
			if (j == p_index)
			{
				continue;
			}
			if (p_polygons[j].contains(poly) == true)
			{
				return true;
			}
		}
		return false;
	}

	static std::vector<spk::Polygon> filterContained(const std::vector<spk::Polygon> &p_polygons)
	{
		std::vector<spk::Polygon> result;
		result.reserve(p_polygons.size());

		for (size_t i = 0; i < p_polygons.size(); ++i)
		{
			if (isContainedByAny(p_polygons, i) == false)
			{
				result.push_back(p_polygons[i]);
			}
		}
		return result;
	}
}

namespace spk
{
	void EdgeMap::addPolygon(const spk::Polygon &p_polygon)
	{
		const spk::Plane polyPlane = p_polygon.plane();
		for (const auto &edge : p_polygon.edges())
		{
			spk::Edge::Identifier id = spk::Edge::Identifier::from(edge);
			auto it = _edges.find(id);
			if (it == _edges.end())
			{
				_edges.emplace(id, Entry{edge, polyPlane, 1});
			}
			else
			{
				it->second.count += 1;
			}
		}
	}

	std::vector<spk::Polygon> EdgeMap::construct() const
	{
		std::vector<BoundaryEdge> boundary = buildBoundary(_edges);

		std::vector<spk::Polygon> polygons;
		polygons.reserve(boundary.size());

		while (boundary.empty() == false)
		{
			BoundaryEdge start = boundary.back();
			boundary.pop_back();

			std::vector<spk::Vector3> loop = traceLoop(boundary, start);
			if (loop.empty() == false)
			{
				polygons.push_back(spk::Polygon::fromLoop(loop));
			}
		}

		return filterContained(polygons);
	}
}
