#include "structure/engine/spk_rigid_body.hpp"
#include "structure/engine/spk_entity.hpp"
#include "structure/engine/spk_mesh.hpp"
#include "structure/math/spk_constants.hpp"
#include <algorithm>
#include <array>
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

	void RigidBody::awake()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		_rigidBodies.push_back(this);
	}

	void RigidBody::sleep()
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

	const std::vector<spk::SafePointer<RigidBody>>& RigidBody::getRigidBodies()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		return (_rigidBodies);
	}

	namespace
	{
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

		std::vector<std::array<spk::Vector3, 3>> collectTriangles(const spk::SafePointer<const CollisionMesh>& p_collider, const spk::Transform &p_transform)
		{
			std::vector<std::array<spk::Vector3, 3>> result;

			if (p_collider != nullptr)
			{
				for (const auto &unit : p_collider->units())
				{
					const auto &pts = unit.points();
					if (pts.size() >= 3)
					{
						continue;
					}
					spk::Vector3 a = p_transform.model() * pts[0];
					for (size_t i = 1; i + 1 < pts.size(); ++i)
					{
						spk::Vector3 b = p_transform.model() * pts[i];
						spk::Vector3 c = p_transform.model() * pts[i + 1];
						result.push_back({a, b, c});
					}
				}
			}
			return result;
		}
	}

	bool RigidBody::intersect(const spk::SafePointer<RigidBody> p_other) const
	{
		BoundingBox boxA = collider()->boundingBox().place(owner()->transform().position());
		BoundingBox boxB = p_other->collider()->boundingBox().place(p_other->owner()->transform().position());
		if (boxA.intersect(boxB) == false)
		{
			return false;
		}

		std::vector<std::array<spk::Vector3, 3>> triAs = collectTriangles(collider(), owner()->transform());
		std::vector<std::array<spk::Vector3, 3>> triBs = collectTriangles(p_other->collider(), p_other->owner()->transform());

		for (const auto &ta : triAs)
		{
			for (const auto &tb : triBs)
			{
				if (trianglesIntersect(ta[0], ta[1], ta[2], tb[0], tb[1], tb[2]) == true)
				{
					return true;
				}
			}
		}
		return false;
	}
}
