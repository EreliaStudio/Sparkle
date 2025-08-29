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
                std::vector<spk::Polygon> polygons;

                std::vector<BoundaryEdge> boundary;
                boundary.reserve(_edges.size());
                for (const auto &[id, entry] : _edges)
                {
                        if (entry.count == 1)
                        {
                                boundary.push_back({entry.edge, entry.plane});
                        }
                }

                while (boundary.empty() == false)
                {
                        std::vector<spk::Vector3> loop;
                        loop.reserve(boundary.size() + 1);

                        BoundaryEdge current = boundary.back();
                        boundary.pop_back();
                        loop.push_back(current.edge.first());
                        loop.push_back(current.edge.second());
                        spk::Plane plane = current.plane;
                        spk::Vector3 cursor = current.edge.second();

                        bool loopClosed = false;

                        while (true)
                        {
                                spk::Vector3 nextCursor;
                                if (findNextEdge(boundary, cursor, nextCursor, plane) == false)
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

                std::vector<spk::Polygon> result;
                result.reserve(polygons.size());
                for (size_t i = 0; i < polygons.size(); ++i)
                {
                        bool contained = false;
                        for (size_t j = 0; j < polygons.size(); ++j)
                        {
                                if (i == j)
                                {
                                        continue;
                                }
                                if (polygons[j].contains(polygons[i]) == true)
                                {
                                        contained = true;
                                        break;
                                }
                        }
                        if (contained == false)
                        {
                                result.push_back(polygons[i]);
                        }
                }

                return result;
        }
}