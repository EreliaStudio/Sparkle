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

		bool haveSharedEdge(const CollisionMesh::Unit &p_a, const CollisionMesh::Unit &p_b)
		{
			for (const auto &edgeA : p_a.edges())
			{
				for (const auto &edgeB : p_b.edges())
				{
					if (edgeA.isInverse(edgeB) == true)
					{
						return (true);
					}
				}
			}
			return (false);
		}

		bool removeSharedOpposite(std::vector<CollisionMesh::Unit> &p_units, const CollisionMesh::Unit &p_poly)
		{
			for (auto it = p_units.begin(); it != p_units.end(); ++it)
			{
				if (it->isCoplanar(p_poly) == true && it->normal() == p_poly.normal().inverse())
				{
					bool shared = it->isOverlapping(p_poly);
					if (shared == false)
					{
						shared = haveSharedEdge(*it, p_poly);
					}
					if (shared == true)
					{
						p_units.erase(it);
						return (true);
					}
				}
			}
			return (false);
		}

		bool fuseWithExisting(std::vector<CollisionMesh::Unit> &p_units, const CollisionMesh::Unit &p_poly)
		{
			for (auto &existing : p_units)
			{
				if (existing.isCoplanar(p_poly) == true && existing.normal() == p_poly.normal() &&
					(existing.isAdjacent(p_poly) == true || existing.isOverlapping(p_poly) == true))
				{
					existing = existing.fuze(p_poly);
					return (true);
				}
			}
			return (false);
		}
	} // namespace

	CollisionMesh CollisionMesh::fromObjMesh(const spk::SafePointer<spk::ObjMesh> &p_mesh)
	{
		CollisionMesh result;
		for (const auto &shape : p_mesh->shapes())
		{
			Unit poly = unitFromShape(shape);
			if (removeSharedOpposite(result._units, poly) == true)
			{
				continue;
			}
			if (fuseWithExisting(result._units, poly) == false)
			{
				result.addUnit(poly);
			}
		}
		return (result);
	}
}
