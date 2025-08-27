#include "structure/engine/spk_collision_mesh.hpp"

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
		CollisionMesh::Unit unitFromShape(const spk::TMesh<spk::Vertex>::Shape &p_shape)
		{
			std::vector<spk::Vector3> points;
			points.reserve(p_shape.vertices.size());
			for (const auto &v : p_shape.vertices)
			{
				points.push_back(v.position);
			}
			return (spk::Polygon::fromLoop(points));
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
		for (const auto &shapeVariant : p_mesh->shapes())
		{
			Unit poly = unitFromShape(shapeVariant);

			if (tryToInsertUnitInMesh(poly, result) == false)
			{
				result.addUnit(poly);			
			}
		}
		return (result);
	}
}
