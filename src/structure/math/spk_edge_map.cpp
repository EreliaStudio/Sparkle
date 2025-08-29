#include "structure/math/spk_edge_map.hpp"

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

			while (boundary.empty() == false)
			{
				bool found = false;
				for (size_t i = 0; i < boundary.size(); ++i)
				{
					const spk::Edge &e = boundary[i];
					if (e.first() == cursor)
					{
						cursor = e.second();
						loop.push_back(cursor);
						boundary.erase(boundary.begin() + i);
						found = true;
						break;
					}
					if (e.second() == cursor)
					{
						cursor = e.first();
						loop.push_back(cursor);
						boundary.erase(boundary.begin() + i);
						found = true;
						break;
					}
				}
				if (found == false)
				{
					break;
				}
				if (cursor == loop.front())
				{
					break;
				}
			}

			polygons.push_back(spk::Polygon::fromLoop(loop));
		}

		return polygons;
	}
}