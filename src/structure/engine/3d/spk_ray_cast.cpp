#include "structure/engine/3d/spk_ray_cast.hpp"
#include "structure/engine/3d/spk_collider.hpp"
#include "structure/math/spk_constants.hpp"

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

	void addHit(
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_dir,
		float p_t,
		const spk::SafePointer<spk::GenericEntity> &p_owner,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		spk::RayCast::Hit hit{};
		hit.position = p_eye + p_dir * p_t;
		hit.entity = p_owner;
		p_hits.push_back(hit);
	}

	void processUnit(
		const spk::CollisionMesh::Unit &p_unit,
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_dir,
		float p_maxDistance,
		const spk::Vector3 &p_offset,
		const spk::SafePointer<spk::GenericEntity> &p_owner,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		const auto &pts = p_unit.points();
		if ((pts.size() >= 3) == true)
		{
			spk::Vector3 base = pts[0] + p_offset;
			for (size_t i = 1; i + 1 < pts.size(); ++i)
			{
				float t = 0.0f;
				spk::Vector3 b = pts[i] + p_offset;
				spk::Vector3 c = pts[i + 1] + p_offset;
				if (rayIntersectsTriangle(p_eye, p_dir, base, b, c, p_maxDistance, t) == true)
				{
					addHit(p_eye, p_dir, t, p_owner, p_hits);
				}
			}
		}
	}

	void processCollider(
		const spk::SafePointer<const spk::CollisionMesh> &p_collisionMesh,
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_dir,
		float p_maxDistance,
		const spk::Vector3 &p_offset,
		const spk::SafePointer<spk::GenericEntity> &p_owner,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		if ((p_collisionMesh == nullptr) == true)
		{
			return;
		}
		for (const auto &unit : p_collisionMesh->units())
		{
			processUnit(unit, p_eye, p_dir, p_maxDistance, p_offset, p_owner, p_hits);
		}
	}

	void processColliderBody(
		const spk::SafePointer<spk::Collider> &p_collider,
		const spk::SafePointer<spk::GameEngine> &p_engine,
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_dir,
		float p_maxDistance,
		std::span<const std::wstring> p_tags,
		spk::BinaryOperator p_binaryOperator,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		if ((p_collider == nullptr) == true)
		{
			return;
		}
		spk::SafePointer<spk::GenericEntity> owner = p_collider->owner();
		if ((owner == nullptr || owner->engine() != p_engine) == true)
		{
			return;
		}
		if ((p_tags.empty() == false))
		{
			if ((owner->containsTags(p_tags, p_binaryOperator) == false))
			{
				return;
			}
		}
		spk::Vector3 offset = owner.upCast<spk::Entity>()->transform().position();
		processCollider(p_collider->collisionMesh(), p_eye, p_dir, p_maxDistance, offset, owner, p_hits);
	}
}

namespace spk
{
	RayCast::Hit RayCast::launch(
		const spk::SafePointer<spk::GameEngine> &p_engine,
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_direction,
		float p_maxDistance,
		std::span<const std::wstring> p_tags,
		spk::BinaryOperator p_binaryOperator)
	{
		std::vector<Hit> hits = launchAll(p_engine, p_eye, p_direction, p_maxDistance, p_tags, p_binaryOperator);
		Hit result{};
		float closest = p_maxDistance;
		for (const auto &hit : hits)
		{
			float dist = (hit.position - p_eye).norm();
			if ((dist < closest) == true)
			{
				closest = dist;
				result = hit;
			}
		}
		return result;
	}

	std::vector<RayCast::Hit> RayCast::launchAll(
		const spk::SafePointer<spk::GameEngine> &p_engine,
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_direction,
		float p_maxDistance,
		std::span<const std::wstring> p_tags,
		spk::BinaryOperator p_binaryOperator)
	{
		std::vector<Hit> hits;
		if ((p_engine == nullptr) == true)
		{
			return hits;
		}
		spk::Vector3 dir = p_direction.normalize();
		std::vector<spk::SafePointer<spk::Collider>> colliders = spk::Collider::getColliders();
		for (const auto &collider : colliders)
		{
			processColliderBody(collider, p_engine, p_eye, dir, p_maxDistance, p_tags, p_binaryOperator, hits);
		}
		return hits;
	}

	RayCast::Hit RayCast::launch(
		const spk::SafePointer<spk::Entity> &p_entity,
		const spk::Vector3 &p_direction,
		float p_maxDistance,
		std::span<const std::wstring> p_tags,
		spk::BinaryOperator p_binaryOperator)
	{
		if ((p_entity == nullptr) == true)
		{
			return {};
		}
		return launch(p_entity->engine(), p_entity->transform().position(), p_direction, p_maxDistance, p_tags, p_binaryOperator);
	}

	std::vector<RayCast::Hit> RayCast::launchAll(
		const spk::SafePointer<spk::Entity> &p_entity,
		const spk::Vector3 &p_direction,
		float p_maxDistance,
		std::span<const std::wstring> p_tags,
		spk::BinaryOperator p_binaryOperator)
	{
		if ((p_entity == nullptr) == true)
		{
			return {};
		}
		return launchAll(p_entity->engine(), p_entity->transform().position(), p_direction, p_maxDistance, p_tags, p_binaryOperator);
	}
}
