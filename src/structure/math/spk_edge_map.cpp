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
	void EdgeMap::addPolygon(const spk::Polygon &p_polygon, bool p_debug)
	{
		if (p_debug == true)
		{
			std::cout << "[EdgeMap] Adding polygon" << std::endl;
			for (const auto &point : p_polygon.points())
			{
				std::cout << "  Point " << point << std::endl;
				if (point.z != 0.0f)
				{
					std::cout << "    Warning: point not on XY plane" << std::endl;
				}
			}
		}

		for (const auto &edge : p_polygon.edges())
		{
			if (p_debug == true)
			{
				std::cout << "  Edge " << edge << std::endl;
			}
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

	std::vector<spk::Polygon> EdgeMap::construct(bool p_debug) const
	{
		if (p_debug == true)
		{
			std::cout << "[EdgeMap] Constructing polygons" << std::endl;
		}

		std::vector<spk::Polygon> polygons;

		std::vector<spk::Edge> boundary;
		boundary.reserve(_edges.size());
		for (const auto &[id, entry] : _edges)
		{
			if (entry.count == 1)
			{
				boundary.push_back(entry.edge);
				if (p_debug == true)
				{
					std::cout << "  Boundary edge " << entry.edge << std::endl;
				}
			}
			else if (p_debug == true)
			{
				std::cout << "  Shared edge " << entry.edge << " count=" << entry.count << std::endl;
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

			if (p_debug == true)
			{
				std::cout << "[EdgeMap] Starting loop with edge " << current << std::endl;
			}

			bool loopClosed = false;

			while (boundary.empty() == false)
			{
				spk::Vector3 nextCursor;
				if (findNextEdge(boundary, cursor, nextCursor) == false)
				{
					if (p_debug == true)
					{
						std::cout << "  No next edge from " << cursor << std::endl;
					}
					break;
				}
				cursor = nextCursor;
				loop.push_back(cursor);
				if (p_debug == true)
				{
					std::cout << "  Moved to " << cursor << std::endl;
				}
				if (cursor == loop.front())
				{
					loopClosed = true;
					if (p_debug == true)
					{
						std::cout << "  Loop closed" << std::endl;
					}
					break;
				}
			}

			if (p_debug == true)
			{
				std::cout << "[EdgeMap] Loop size " << loop.size() << std::endl;
			}

			if (loopClosed == true && loop.size() > 2)
			{
				if (loop.back() != loop.front())
				{
					loop.push_back(loop.front());
				}
				polygons.push_back(spk::Polygon::fromLoop(loop));

				if (p_debug == true)
				{
					std::cout << "[EdgeMap] Constructed polygon:" << std::endl;
					for (const auto &point : loop)
					{
						std::cout << "    " << point << std::endl;
					}
				}
			}
			else if (p_debug == true)
			{
				std::cout << "[EdgeMap] Ignoring open loop" << std::endl;
			}
		}

		if (p_debug == true)
		{
			std::cout << "[EdgeMap] Constructed " << polygons.size() << " polygon(s)" << std::endl;
		}

		return polygons;
	}
}