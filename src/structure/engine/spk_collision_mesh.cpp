#include "structure/engine/spk_collision_mesh.hpp"
#include "structure/math/spk_reference_frame.hpp"
#include <unordered_map>

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
		spk::CollisionMesh result;

		std::unordered_map<spk::Plane::Identifier, std::vector<spk::Polygon>> polygonCollection;

		for (const auto &shape : p_mesh->shapes())
		{
			spk::CollisionMesh::Unit polygon;
			
			for (const auto &vertex : shape.points)
			{
				polygon.points.push_back(vertex.position);
			}

			if (polygon.isPlanar() == true)
			{
				spk::Plane tmpPlane = polygon.plane();

				polygonCollection[spk::Plane::Identifier::from(tmpPlane)].push_back(polygon);
				//result.addUnit(polygon);
			}
		}

		for (const auto& [key, polygons] : polygonCollection)
		{
			for (const auto& polygon : polygons)
			{
				result.addUnit(polygon);
			}
		}

		return (result);
	}
}
