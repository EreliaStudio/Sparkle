#include "structure/engine/spk_ray_cast.hpp"
#include "spk_constants.hpp"
#include "structure/engine/spk_rigid_body.hpp"

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
		if (p_engine == nullptr)
		{
			return hits;
		}
		spk::Vector3 dir = p_direction.normalize();

		std::vector<spk::SafePointer<spk::RigidBody>> bodies = spk::RigidBody::getRigidBodies();
		for (const auto &body : bodies)
		{
			if (body == nullptr)
			{
				continue;
			}

			spk::SafePointer<spk::Entity> owner = body->owner();
			if (owner == nullptr || owner->engine() != p_engine)
			{
				continue;
			}
			spk::Vector3 offset = owner->transform().position();

			for (const auto &collider : body->colliders())
			{
				if (collider == nullptr)
				{
					continue;
				}

				for (const auto &unit : collider->units())
				{
					for (const auto &shape : unit.shapes())
					{
						if (std::holds_alternative<spk::TMesh<spk::Vector3>::Triangle>(shape) == true)
						{
							const auto &tri = std::get<spk::TMesh<spk::Vector3>::Triangle>(shape);
							float t = 0.0f;
							if (rayIntersectsTriangle(p_eye, dir, tri.a + offset, tri.b + offset, tri.c + offset, p_maxDistance, t) == true)
							{
								Hit hit;
								hit.position = p_eye + dir * t;
								hit.entity = owner;
								hits.push_back(hit);
							}
						}
						else
						{
							const auto &quad = std::get<spk::TMesh<spk::Vector3>::Quad>(shape);
							float t = 0.0f;
							if (rayIntersectsTriangle(p_eye, dir, quad.a + offset, quad.b + offset, quad.c + offset, p_maxDistance, t) == true)
							{
								Hit hit;
								hit.position = p_eye + dir * t;
								hit.entity = owner;
								hits.push_back(hit);
							}
							if (rayIntersectsTriangle(p_eye, dir, quad.a + offset, quad.c + offset, quad.d + offset, p_maxDistance, t) == true)
							{
								Hit hit;
								hit.position = p_eye + dir * t;
								hit.entity = owner;
								hits.push_back(hit);
							}
						}
					}
				}
			}
		}
		return hits;
	}

	RayCast::Hit RayCast::launch(const spk::SafePointer<spk::Entity> &p_entity, const spk::Vector3 &p_direction, float p_maxDistance)
	{
		if (p_entity == nullptr)
		{
			return {};
		}
		return launch(p_entity->engine(), p_entity->transform().position(), p_direction, p_maxDistance);
	}

	std::vector<RayCast::Hit> RayCast::launchAll(const spk::SafePointer<spk::Entity> &p_entity, const spk::Vector3 &p_direction, float p_maxDistance)
	{
		if (p_entity == nullptr)
		{
			return {};
		}
		return launchAll(p_entity->engine(), p_entity->transform().position(), p_direction, p_maxDistance);
	}
}
