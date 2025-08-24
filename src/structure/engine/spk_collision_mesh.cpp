#include "structure/engine/spk_collision_mesh.hpp"

#include <algorithm>
#include <variant>

namespace spk
{
	void CollisionMesh::addPolygon(const spk::Polygon &p_polygon)
	{
		_polygons.push_back(p_polygon);
	}

	const std::vector<spk::Polygon> &CollisionMesh::polygons() const
	{
		return _polygons;
	}

	CollisionMesh CollisionMesh::fromObjMesh(const spk::SafePointer<spk::ObjMesh> &p_mesh)
	{
		std::vector<spk::Polygon> polys;
		for (const auto &shapeVariant : p_mesh->shapes())
		{
			spk::Polygon poly;
			if (std::holds_alternative<spk::ObjMesh::Quad>(shapeVariant) == true)
			{
				const auto &q = std::get<spk::ObjMesh::Quad>(shapeVariant);
				poly.addQuad(q.a.position, q.b.position, q.c.position, q.d.position);
			}
			else
			{
				const auto &t = std::get<spk::ObjMesh::Triangle>(shapeVariant);
				poly.addTriangle(t.a.position, t.b.position, t.c.position);
			}
			polys.push_back(poly);
		}

		bool merged = true;
		while (merged == true)
		{
			merged = false;
			for (size_t i = 0; i < polys.size() && merged == false; ++i)
			{
				for (size_t j = i + 1; j < polys.size(); ++j)
				{
					if (polys[i].canInsert(polys[j]) == true)
					{
						polys[i].addPolygon(polys[j]);
						polys.erase(polys.begin() + j);
						merged = true;
						break;
					}
				}
			}
		}

		CollisionMesh result;
		for (auto &poly : polys)
		{
			if (poly.isConvex() == true)
			{
				result.addPolygon(poly);
			}
			else
			{
				auto parts = poly.split();
				for (auto &part : parts)
				{
					result.addPolygon(part);
				}
			}
		}
		return result;
	}
}
