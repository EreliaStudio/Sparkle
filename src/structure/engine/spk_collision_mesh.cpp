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
		spk::CollisionMesh result;
		for (const auto &shape : p_mesh->shapes())
		{
			spk::CollisionMesh::Unit unit;
			for (const auto &vertex : shape.points)
			{
				unit.points.push_back(vertex.position);
			}
			result.addUnit(unit);
		}
		return (result);
	}
}
