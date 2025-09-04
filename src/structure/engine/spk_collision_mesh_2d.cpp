#include "structure/engine/spk_collision_mesh_2d.hpp"

namespace spk
{
	void CollisionMesh2D::addUnit(const Unit &p_unit)
	{
		_units.push_back(p_unit);

		const spk::BoundingBox2D &unitBoundingBox = p_unit.boundingBox();
		_boundingBox.addPoint(unitBoundingBox.min);
		_boundingBox.addPoint(unitBoundingBox.max);
	}

	const std::vector<CollisionMesh2D::Unit> &CollisionMesh2D::units() const
	{
		return (_units);
	}

	const spk::BoundingBox2D &CollisionMesh2D::boundingBox() const
	{
		return (_boundingBox);
	}

	bool CollisionMesh2D::intersect(const CollisionMesh2D &p_other) const
	{
		if (boundingBox().intersect(p_other.boundingBox()) == false)
		{
			return false;
		}

		for (const auto &unitA : units())
		{
			for (const auto &unitB : p_other.units())
			{
				if (unitA.isOverlapping(unitB) == true)
				{
					return true;
				}
			}
		}

		return false;
	}

	CollisionMesh2D CollisionMesh2D::fromMesh(const spk::SafePointer<const spk::Mesh2D> &p_mesh)
	{
		CollisionMesh2D result;
		if ((p_mesh == nullptr) == true)
		{
			return (result);
		}

		const auto &buffer = p_mesh->buffer();
		const auto &vertices = buffer.vertices;
		const auto &indexes = buffer.indexes;
		for (size_t i = 0; i + 2 < indexes.size(); i += 3)
		{
			Unit unit({vertices[indexes[i]].position, vertices[indexes[i + 1]].position, vertices[indexes[i + 2]].position});
			result.addUnit(unit);
		}
		return (result);
	}
}
