#include "structure/engine/spk_collision_mesh.hpp"

#include <variant>

namespace spk
{
	void CollisionMesh::addUnit(const Unit &p_unit)
	{
		_units.push_back(p_unit);
	}

	std::vector<CollisionMesh::Unit> &CollisionMesh::units()
	{
		return (_units);
	}

	const std::vector<CollisionMesh::Unit> &CollisionMesh::units() const
	{
		return (_units);
	}

	namespace
	{
		CollisionMesh::Unit unitFromVariant(const std::variant<spk::TMesh<spk::Vertex>::Triangle, spk::TMesh<spk::Vertex>::Quad> &p_shape)
		{
			if (std::holds_alternative<spk::ObjMesh::Quad>(p_shape) == true)
			{
				const auto &q = std::get<spk::ObjMesh::Quad>(p_shape);
				return (spk::Polygon::makeSquare(q.a.position, q.b.position, q.c.position, q.d.position));
			}
			const auto &t = std::get<spk::ObjMesh::Triangle>(p_shape);
			return (spk::Polygon::makeTriangle(t.a.position, t.b.position, t.c.position));
		}

		bool tryToInsertUnitInMesh(const CollisionMesh::Unit& p_newPolygon, CollisionMesh& p_currentResult)
		{
			for (auto& existingPoly : p_currentResult.units())
			{
				if (existingPoly.isCoplanar(p_newPolygon) == true &&
					(existingPoly.isAdjacent(p_newPolygon) == true || existingPoly.isOverlapping(p_newPolygon) == true))
				{
					// We can fuse them
					Polygon fused = existingPoly.fuze(p_newPolygon);
					if (fused.isConvex() == false)
					{
						return (false);
					}
					
					existingPoly = fused;
					
					return (true);
				}
			}

			return (false);
		}
	} // namespace

	CollisionMesh CollisionMesh::fromObjMesh(const spk::SafePointer<spk::ObjMesh> &p_mesh)
	{
		CollisionMesh result;

		// for (const auto &shapeVariant : p_mesh->shapes())
		// {
		// 	Unit poly = unitFromVariant(shapeVariant);
		// 	if (removeSharedOpposite(result._units, poly) == true)
		// 	{
		// 		continue;
		// 	}
		// 	if (fuseWithExisting(result._units, poly) == false)
		// 	{
		// 		result.addUnit(poly);
		// 	}
		// }
		for (const auto &shapeVariant : p_mesh->shapes())
		{
			Unit poly = unitFromVariant(shapeVariant);

			if (tryToInsertUnitInMesh(poly, result) == false)
			{
				result.addUnit(poly);			
			}
		}
		return (result);
	}
}
