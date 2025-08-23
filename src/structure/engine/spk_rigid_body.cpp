#include "structure/engine/spk_rigid_body.hpp"
#include "spk_constants.hpp"
#include "structure/engine/spk_entity.hpp"
#include <algorithm>
#include <variant>
#include <vector>

namespace spk
{
	RigidBody::RigidBody(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	RigidBody::~RigidBody()
	{
		stop();
	}

	void RigidBody::start()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		_rigidBodies.push_back(this);
	}

	void RigidBody::stop()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		auto it = std::remove_if(
			_rigidBodies.begin(), _rigidBodies.end(), [this](const spk::SafePointer<RigidBody> &p_ptr) { return (p_ptr.get() == this); });
		_rigidBodies.erase(it, _rigidBodies.end());
	}

	void RigidBody::setCollider(const spk::SafePointer<const spk::CollisionMesh> &p_collider)
	{
		_collider = p_collider;
	}

	const spk::SafePointer<const spk::CollisionMesh> &RigidBody::collider() const
	{
		return (_collider);
	}

	RigidBody::BoundingBox RigidBody::_computeLocalBoundingBox() const
	{
		BoundingBox result{};
		bool initialized = false;

		const auto &collider = _collider;
		if (collider != nullptr)
		{
			for (const auto &unit : collider->units())
			{
				const auto &vertices = unit.buffer().vertices;
				for (const auto &vertex : vertices)
				{
					if (initialized == false)
					{
						result.min = vertex;
						result.max = vertex;
						initialized = true;
					}
					else
					{
						result.min = spk::Vector3::min(result.min, vertex);
						result.max = spk::Vector3::max(result.max, vertex);
					}
				}
			}
		}

		return (result);
	}

	RigidBody::BoundingBox RigidBody::boundingBox() const
	{
		BoundingBox box = _computeLocalBoundingBox();
		if (owner() != nullptr)
		{
			const spk::Vector3 &pos = owner()->transform().position();
			box.min += pos;
			box.max += pos;
		}
		return (box);
	}

	std::vector<spk::SafePointer<RigidBody>> RigidBody::getRigidBodies()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		return (_rigidBodies);
	}

	namespace
	{
		RigidBody::BoundingBox computeBoundingBox(const RigidBody *p_body, const spk::Matrix4x4 &p_model)
		{
			RigidBody::BoundingBox result{};
			bool initialized = false;
			const auto &collider = p_body->collider();
			if (collider != nullptr)
			{
				for (const auto &unit : collider->units())
				{
					for (const auto &vertex : unit.buffer().vertices)
					{
						spk::Vector3 transformed = p_model * vertex;
						if (initialized == false)
						{
							result.min = transformed;
							result.max = transformed;
							initialized = true;
						}
						else
						{
							result.min = spk::Vector3::min(result.min, transformed);
							result.max = spk::Vector3::max(result.max, transformed);
						}
					}
				}
			}
			return result;
		}

		bool boxesIntersect(const RigidBody::BoundingBox &p_a, const RigidBody::BoundingBox &p_b)
		{
			return (
				(p_a.min.x <= p_b.max.x && p_a.max.x >= p_b.min.x) == true && (p_a.min.y <= p_b.max.y && p_a.max.y >= p_b.min.y) == true &&
				(p_a.min.z <= p_b.max.z && p_a.max.z >= p_b.min.z) == true);
		}

		bool rayIntersectsTriangle(
			const spk::Vector3 &p_origin,
			const spk::Vector3 &p_direction,
			const spk::Vector3 &p_v0,
			const spk::Vector3 &p_v1,
			const spk::Vector3 &p_v2,
			float p_maxDistance,
			float &p_t)
		{
			spk::Vector3 edge1 = p_v1 - p_v0;
			spk::Vector3 edge2 = p_v2 - p_v0;
			spk::Vector3 h = p_direction.cross(edge2);
			float a = edge1.dot(h);
			if ((a > -spk::Constants::pointPrecision && a < spk::Constants::pointPrecision) == true)
			{
				return false;
			}
			float f = 1.0f / a;
			spk::Vector3 s = p_origin - p_v0;
			float u = f * s.dot(h);
			if ((u < 0.0f || u > 1.0f) == true)
			{
				return false;
			}
			spk::Vector3 q = s.cross(edge1);
			float v = f * p_direction.dot(q);
			if ((v < 0.0f || u + v > 1.0f) == true)
			{
				return false;
			}
			float t = f * edge2.dot(q);
			if ((t > spk::Constants::pointPrecision && t <= p_maxDistance) == true)
			{
				p_t = t;
				return true;
			}
			return false;
		}

		bool segmentIntersectsTriangle(
			const spk::Vector3 &p_start, const spk::Vector3 &p_end, const spk::Vector3 &p_v0, const spk::Vector3 &p_v1, const spk::Vector3 &p_v2)
		{
			spk::Vector3 dir = p_end - p_start;
			float length = dir.norm();
			if ((length == 0.0f) == true)
			{
				return false;
			}
			float t = 0.0f;
			return rayIntersectsTriangle(p_start, dir.normalize(), p_v0, p_v1, p_v2, length, t);
		}

		bool pointInTriangle(const spk::Vector3 &p_point, const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c)
		{
			spk::Vector3 v0 = p_b - p_a;
			spk::Vector3 v1 = p_c - p_a;
			spk::Vector3 v2 = p_point - p_a;

			float d00 = v0.dot(v0);
			float d01 = v0.dot(v1);
			float d11 = v1.dot(v1);
			float d20 = v2.dot(v0);
			float d21 = v2.dot(v1);
			float denom = d00 * d11 - d01 * d01;
			if ((denom == 0.0f) == true)
			{
				return false;
			}
			float v = (d11 * d20 - d01 * d21) / denom;
			float w = (d00 * d21 - d01 * d20) / denom;
			float u = 1.0f - v - w;
			return ((u >= 0.0f && v >= 0.0f && w >= 0.0f) == true);
		}

		bool trianglesIntersect(
			const spk::Vector3 &p_a0,
			const spk::Vector3 &p_a1,
			const spk::Vector3 &p_a2,
			const spk::Vector3 &p_b0,
			const spk::Vector3 &p_b1,
			const spk::Vector3 &p_b2)
		{
			if (segmentIntersectsTriangle(p_a0, p_a1, p_b0, p_b1, p_b2) == true || segmentIntersectsTriangle(p_a1, p_a2, p_b0, p_b1, p_b2) == true ||
				segmentIntersectsTriangle(p_a2, p_a0, p_b0, p_b1, p_b2) == true)
			{
				return true;
			}
			if (segmentIntersectsTriangle(p_b0, p_b1, p_a0, p_a1, p_a2) == true || segmentIntersectsTriangle(p_b1, p_b2, p_a0, p_a1, p_a2) == true ||
				segmentIntersectsTriangle(p_b2, p_b0, p_a0, p_a1, p_a2) == true)
			{
				return true;
			}
			if (pointInTriangle(p_a0, p_b0, p_b1, p_b2) == true || pointInTriangle(p_b0, p_a0, p_a1, p_a2) == true)
			{
				return true;
			}
			return false;
		}

		std::vector<spk::TMesh<spk::Vector3>::Triangle> collectTriangles(const RigidBody *p_body, const spk::Matrix4x4 &p_transform)
		{
			std::vector<spk::TMesh<spk::Vector3>::Triangle> result;
			const auto &collider = p_body->collider();
			if ((collider == nullptr) == false)
			{
				for (const auto &unit : collider->units())
				{
					for (const auto &shape : unit.shapes())
					{
						if (std::holds_alternative<spk::TMesh<spk::Vector3>::Triangle>(shape) == true)
						{
							auto tri = std::get<spk::TMesh<spk::Vector3>::Triangle>(shape);
							tri.a = p_transform * tri.a;
							tri.b = p_transform * tri.b;
							tri.c = p_transform * tri.c;
							result.push_back(tri);
						}
						else
						{
							auto quad = std::get<spk::TMesh<spk::Vector3>::Quad>(shape);
							result.push_back({p_transform * quad.a, p_transform * quad.b, p_transform * quad.c});
							result.push_back({p_transform * quad.a, p_transform * quad.c, p_transform * quad.d});
						}
					}
				}
			}
			return result;
		}
	}

	bool RigidBody::intersect(const RigidBody *p_a, const spk::Matrix4x4 &p_transformA, const RigidBody *p_b, const spk::Matrix4x4 &p_transformB)
	{
		BoundingBox boxA = computeBoundingBox(p_a, p_transformA);
		BoundingBox boxB = computeBoundingBox(p_b, p_transformB);
		if (boxesIntersect(boxA, boxB) == false)
		{
			return false;
		}

		std::vector<spk::TMesh<spk::Vector3>::Triangle> triAs = collectTriangles(p_a, p_transformA);
		std::vector<spk::TMesh<spk::Vector3>::Triangle> triBs = collectTriangles(p_b, p_transformB);

		for (const auto &ta : triAs)
		{
			for (const auto &tb : triBs)
			{
				if (trianglesIntersect(ta.a, ta.b, ta.c, tb.a, tb.b, tb.c) == true)
				{
					return true;
				}
			}
		}
		return false;
	}
}
