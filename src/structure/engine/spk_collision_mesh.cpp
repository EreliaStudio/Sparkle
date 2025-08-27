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

		struct ReferenceFrameHash
		{
			std::size_t operator()(const spk::ReferenceFrame &p_frame) const noexcept
			{
				spk::Vector3 normal = p_frame.zAxis().normalize();
				if ((normal.x < 0.0f) == true || (FLOAT_EQ(normal.x, 0.0f) == true &&
												  ((normal.y < 0.0f) == true || (FLOAT_EQ(normal.y, 0.0f) == true && (normal.z < 0.0f) == true))))
				{
					normal = normal * -1.0f;
				}
				const float d = normal.dot(p_frame.origin());
				std::size_t h1 = std::hash<spk::Vector3>()(normal);
				std::size_t h2 = std::hash<float>()(d);
				return h1 ^ (h2 << 1);
			}
		};

		struct ReferenceFrameEqual
		{
			bool operator()(const spk::ReferenceFrame &p_a, const spk::ReferenceFrame &p_b) const noexcept
			{
				spk::Vector3 normalA = p_a.zAxis().normalize();
				spk::Vector3 normalB = p_b.zAxis().normalize();

				if ((normalA.x < 0.0f) == true || (FLOAT_EQ(normalA.x, 0.0f) == true &&
												   ((normalA.y < 0.0f) == true || (FLOAT_EQ(normalA.y, 0.0f) == true && (normalA.z < 0.0f) == true))))
				{
					normalA = normalA * -1.0f;
				}
				if ((normalB.x < 0.0f) == true || (FLOAT_EQ(normalB.x, 0.0f) == true &&
												   ((normalB.y < 0.0f) == true || (FLOAT_EQ(normalB.y, 0.0f) == true && (normalB.z < 0.0f) == true))))
				{
					normalB = normalB * -1.0f;
				}
				if ((normalA != normalB) == true)
				{
					return false;
				}
				const float dA = normalA.dot(p_a.origin());
				const float dB = normalB.dot(p_b.origin());
				return (FLOAT_EQ(dA, dB) == true);
			}
		};

		std::unordered_map<spk::ReferenceFrame, std::vector<spk::Polygon>, ReferenceFrameHash, ReferenceFrameEqual> polygonsByFrame;

		for (const auto &shape : p_mesh->shapes())
		{
			spk::CollisionMesh::Unit polygon;
			for (const auto &vertex : shape.points)
			{
				polygon.points.push_back(vertex.position);
			}
			if (polygon.isPlanar() == true)
			{
				spk::ReferenceFrame frame(polygon.plane());
				spk::Polygon localPolygon = frame.convertTo(polygon);
				polygonsByFrame[frame].push_back(localPolygon);
			}
		}

		for (const auto &[frame, polygons] : polygonsByFrame)
		{
			for (const auto &localPolygon : polygons)
			{
				spk::Polygon worldPolygon = frame.convertFrom(localPolygon);
				result.addUnit(worldPolygon);
			}
		}

		return (result);
	}
}
