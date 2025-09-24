#include "structure/engine/3d/spk_collision_mesh.hpp"
#include "structure/math/spk_edge_map.hpp"
#include "structure/math/spk_plane.hpp"
#include "structure/math/spk_reference_frame.hpp"
#include <unordered_map>

namespace spk
{
	void CollisionMesh::addUnit(const Unit &p_unit)
	{
		_units.push_back(p_unit);

		const spk::BoundingBox &unitBoundingBox = p_unit.boundingBox();
		_boundingBox.addPoint(unitBoundingBox.min);
		_boundingBox.addPoint(unitBoundingBox.max);
	}

	const std::vector<CollisionMesh::Unit> &CollisionMesh::units() const
	{
		return (_units);
	}

	const spk::BoundingBox &CollisionMesh::boundingBox() const
	{
		return (_boundingBox);
	}

	bool CollisionMesh::intersect(const CollisionMesh &p_other) const
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

	CollisionMesh CollisionMesh::fromObjMesh(const spk::SafePointer<spk::ObjMesh> &p_mesh)
	{
		spk::CollisionMesh result;

		std::unordered_map<spk::Plane::Identifier, spk::EdgeMap> polygonCollection;

		for (const auto &shape : p_mesh->shapes())
		{
			std::vector<spk::Vector3> points;
			points.reserve(shape.points.size());
			for (const auto &point : shape.points)
			{
				points.push_back(point.position);
			}

			if (points.size() < 3)
			{
				continue;
			}

			spk::CollisionMesh::Unit polygon = spk::Polygon::fromLoop(points);

			if (polygon.isPlanar() == true)
			{
				spk::Plane tmpPlane = polygon.plane();

				polygonCollection[spk::Plane::Identifier::from(tmpPlane)].addPolygon(polygon);
			}
		}

		for (auto &[key, edgeMap] : polygonCollection)
		{
			for (const auto &polygon : edgeMap.construct())
			{
				if (polygon.isConvex() == true)
				{
					result.addUnit(polygon);
				}
				else
				{
					for (const auto &convexPolygon : polygon.splitIntoConvex())
					{
						result.addUnit(convexPolygon);
					}
				}
			}
		}

		return (result);
	}
}