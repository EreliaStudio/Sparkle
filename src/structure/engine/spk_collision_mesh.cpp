#include "structure/engine/spk_collision_mesh.hpp"

namespace spk
{
	void CollisionMesh::addUnit(const Unit &p_unit)
	{
		_units.push_back(p_unit);
	}

	const std::vector<CollisionMesh::Unit> &CollisionMesh::units() const
	{
		return (_units);
	}

	CollisionMesh CollisionMesh::fromObjMesh(const spk::SafePointer<spk::ObjMesh> &p_mesh)
	{
		std::vector<spk::Polygon> polys;
		try
		{
			for (const auto &shapeVariant : p_mesh->shapes())
			{
				spk::Polygon poly;
				if (std::holds_alternative<spk::ObjMesh::Quad>(shapeVariant) == true)
				{
					const auto &q = std::get<spk::ObjMesh::Quad>(shapeVariant);
					poly = spk::Polygon::makeSquare(q.a.position, q.b.position, q.c.position, q.d.position);
				}
				else
				{
					const auto &t = std::get<spk::ObjMesh::Triangle>(shapeVariant);
					poly = spk::Polygon::makeTri(t.a.position, t.b.position, t.c.position);
				}
				polys.push_back(poly);
			}
		}
		catch(const std::exception& e)
		{
			PROPAGATE_ERROR("While extracting all polygons from input", e);
		}

		try
		{
		bool merged = true;
		while (merged == true)
		{
			merged = false;
			for (size_t i = 0; i < polys.size(); ++i)
			{
				for (size_t j = i + 1; j < polys.size();)
				{
					if (polys[i].isCoplanar(polys[j]) == true && (polys[i].isAdjacent(polys[j]) == true || polys[i].isOverlapping(polys[j]) == true))
					{
						polys[i] = polys[i].fuze(polys[j]);
						polys.erase(polys.begin() + j);
						merged = true;
					}
					else
					{
						++j;
					}
				}
			}
		}
		}
		catch(const std::exception& e)
		{
			PROPAGATE_ERROR("While merging the different planes all polygons from polygon list", e);
		}

		CollisionMesh result;
		try
		{
			for (const auto &poly : polys)
			{
				result.addUnit(poly);
			}
		}
		catch(const std::exception& e)
		{
			PROPAGATE_ERROR("While constructing collision mesh output", e);
		}
		return (result);
	}
}
