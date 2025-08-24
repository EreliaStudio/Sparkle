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
		for (const auto &shapeVariant : p_mesh->shapes())
		{
			spk::CollisionMesh::Unit unit;
			if (std::holds_alternative<spk::ObjMesh::Quad>(shapeVariant) == true)
			{
				const auto &q = std::get<spk::ObjMesh::Quad>(shapeVariant);
				unit.points = {q.a.position, q.b.position, q.c.position, q.d.position};
			}
			else
			{
				const auto &t = std::get<spk::ObjMesh::Triangle>(shapeVariant);
				unit.points = {t.a.position, t.b.position, t.c.position};
			}
			result.addUnit(unit);
		}
		return (result);
	}
}
