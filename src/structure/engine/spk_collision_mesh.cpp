#include "structure/engine/spk_collision_mesh.hpp"
#include <algorithm>

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
		try
		{
			auto isDegenerate = [](const spk::Polygon &p_poly) -> bool
			{
				try
				{
					const auto &edgesRef = p_poly.edges();
					if (edgesRef.size() < 3)
					{
						return true;
					}
					spk::Vector3 n = edgesRef[0].direction().cross(edgesRef[1].direction());
					for (size_t i = 2; (n == spk::Vector3(0, 0, 0)) == true && i < edgesRef.size(); ++i)
					{
						n = edgesRef[0].direction().cross(edgesRef[i].direction());
					}
					return ((n == spk::Vector3(0, 0, 0)) == true);
				} catch (...)
				{
					return true;
				}
			};

			std::vector<spk::Polygon> polys;
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
				if (isDegenerate(poly) == false)
				{
					polys.push_back(poly);
				}
			}

			bool merged = true;
			while (merged == true)
			{
				polys.erase(std::remove_if(polys.begin(), polys.end(), isDegenerate), polys.end());
				merged = false;
				for (size_t i = 0; i < polys.size(); ++i)
				{
					for (size_t j = i + 1; j < polys.size(); ++j)
					{
						if (polys[i].isCoplanar(polys[j]) == true &&
							(polys[i].isAdjacent(polys[j]) == true || polys[i].isOverlapping(polys[j]) == true))
						{
							polys[i] = polys[i].fuze(polys[j]);
							polys.erase(polys.begin() + j);
							merged = true;
							break;
						}
					}
					if (merged == true)
					{
						break;
					}
				}
			}

			CollisionMesh result;
			for (const auto &poly : polys)
			{
				result.addUnit(poly);
			}
			return result;
		} catch (const std::exception &e)
		{
			GENERATE_ERROR(std::string("CollisionMesh::fromObjMesh - ") + e.what());
		} catch (...)
		{
			GENERATE_ERROR("CollisionMesh::fromObjMesh - Unknown error");
		}
	}
}
