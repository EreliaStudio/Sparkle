#include "structure/engine/spk_ray_cast.hpp"
#include "structure/engine/spk_rigid_body.hpp"
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
		const spk::SafePointer<spk::Entity> &p_owner,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		spk::RayCast::Hit hit{};
		hit.position = p_eye + p_dir * p_t;
		hit.entity = p_owner;
		p_hits.push_back(hit);
	}

	struct Triangle
	{
		spk::Vector3 a;
		spk::Vector3 b;
		spk::Vector3 c;
	};

	struct Quad
	{
		spk::Vector3 a;
		spk::Vector3 b;
		spk::Vector3 c;
		spk::Vector3 d;
	};

	void processTriangle(
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_dir,
		float p_maxDistance,
		const spk::Vector3 &p_offset,
		const Triangle &p_tri,
		const spk::SafePointer<spk::Entity> &p_owner,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		float t = 0.0f;
		if (rayIntersectsTriangle(p_eye, p_dir, p_tri.a + p_offset, p_tri.b + p_offset, p_tri.c + p_offset, p_maxDistance, t) == true)
		{
			addHit(p_eye, p_dir, t, p_owner, p_hits);
		}
	}

	void processQuad(
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_dir,
		float p_maxDistance,
		const spk::Vector3 &p_offset,
		const Quad &p_quad,
		const spk::SafePointer<spk::Entity> &p_owner,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		processTriangle(p_eye, p_dir, p_maxDistance, p_offset, {p_quad.a, p_quad.b, p_quad.c}, p_owner, p_hits);
		processTriangle(p_eye, p_dir, p_maxDistance, p_offset, {p_quad.a, p_quad.c, p_quad.d}, p_owner, p_hits);
	}

	void processUnit(
		const spk::CollisionMesh::Unit &p_unit,
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_dir,
		float p_maxDistance,
		const spk::Vector3 &p_offset,
		const spk::SafePointer<spk::Entity> &p_owner,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		if ((p_unit.points.size() == 3) == true)
		{
			Triangle tri{p_unit.points[0], p_unit.points[1], p_unit.points[2]};
			processTriangle(p_eye, p_dir, p_maxDistance, p_offset, tri, p_owner, p_hits);
		}
		else if ((p_unit.points.size() == 4) == true)
		{
			Quad quad{p_unit.points[0], p_unit.points[1], p_unit.points[2], p_unit.points[3]};
			processQuad(p_eye, p_dir, p_maxDistance, p_offset, quad, p_owner, p_hits);
		}
	}

	void processCollider(
		const spk::SafePointer<const spk::CollisionMesh> &p_collider,
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_dir,
		float p_maxDistance,
		const spk::Vector3 &p_offset,
		const spk::SafePointer<spk::Entity> &p_owner,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		if ((p_collider == nullptr) == true)
		{
			return;
		}
		for (const auto &unit : p_collider->units())
		{
			processUnit(unit, p_eye, p_dir, p_maxDistance, p_offset, p_owner, p_hits);
		}
	}

	void processBody(
		const spk::SafePointer<spk::RigidBody> &p_body,
		const spk::SafePointer<spk::GameEngine> &p_engine,
		const spk::Vector3 &p_eye,
		const spk::Vector3 &p_dir,
		float p_maxDistance,
		std::vector<spk::RayCast::Hit> &p_hits)
	{
		if ((p_body == nullptr) == true)
		{
			return;
		}
		spk::SafePointer<spk::Entity> owner = p_body->owner();
		if ((owner == nullptr || owner->engine() != p_engine) == true)
		{
			return;
		}
		spk::Vector3 offset = owner->transform().position();
		processCollider(p_body->collider(), p_eye, p_dir, p_maxDistance, offset, owner, p_hits);
	}
}

namespace spk
{
	RayCast::Hit RayCast::launch(
		const spk::SafePointer<spk::GameEngine> &p_engine, const spk::Vector3 &p_eye, const spk::Vector3 &p_direction, float p_maxDistance)
	{
		std::vector<Hit> hits = launchAll(p_engine, p_eye, p_direction, p_maxDistance);
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
		const spk::SafePointer<spk::GameEngine> &p_engine, const spk::Vector3 &p_eye, const spk::Vector3 &p_direction, float p_maxDistance)
	{
		std::vector<Hit> hits;
		if ((p_engine == nullptr) == true)
		{
			return hits;
		}
		spk::Vector3 dir = p_direction.normalize();
		std::vector<spk::SafePointer<spk::RigidBody>> bodies = spk::RigidBody::getRigidBodies();
		for (const auto &body : bodies)
		{
			processBody(body, p_engine, p_eye, dir, p_maxDistance, hits);
		}
		return hits;
	}

	RayCast::Hit RayCast::launch(const spk::SafePointer<spk::Entity> &p_entity, const spk::Vector3 &p_direction, float p_maxDistance)
	{
		if ((p_entity == nullptr) == true)
		{
			return {};
		}
		return launch(p_entity->engine(), p_entity->transform().position(), p_direction, p_maxDistance);
	}

	std::vector<RayCast::Hit> RayCast::launchAll(const spk::SafePointer<spk::Entity> &p_entity, const spk::Vector3 &p_direction, float p_maxDistance)
	{
		if ((p_entity == nullptr) == true)
		{
			return {};
		}
		return launchAll(p_entity->engine(), p_entity->transform().position(), p_direction, p_maxDistance);
	}
}
