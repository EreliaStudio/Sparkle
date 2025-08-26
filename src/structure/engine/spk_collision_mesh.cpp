#include "structure/engine/spk_collision_mesh.hpp"

#include <variant>

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
		for (const auto &shapeVariant : p_mesh->shapes())
		{
			Unit poly = unitFromVariant(shapeVariant);
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
