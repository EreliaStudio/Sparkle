#include "structure/engine/spk_collision_mesh.hpp"
#include "structure/math/spk_plane.hpp"
#include "structure/math/spk_edge_map.hpp"
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

				bool isPlaneXYAtOrigin = (spk::Plane::Identifier::from(tmpPlane).normal == spk::Vector3(0, 0, 1) && spk::Plane::Identifier::from(tmpPlane).dotValue == 0.0f);
				polygonCollection[spk::Plane::Identifier::from(tmpPlane)].addPolygon(polygon);
			}
		}

		for (auto &[key, edgeMap] : polygonCollection)
		{
			bool isPlaneXYAtOrigin = (key.normal == spk::Vector3(0, 0, 1) && key.dotValue == 0.0f);
			for (const auto &polygon : edgeMap.construct(isPlaneXYAtOrigin))
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