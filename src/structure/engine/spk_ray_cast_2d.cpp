#include "structure/engine/spk_ray_cast_2d.hpp"
#include "structure/engine/spk_camera_component.hpp"
#include "structure/engine/spk_game_engine.hpp"
#include "structure/engine/spk_rigid_body_2d.hpp"
#include "structure/math/spk_constants.hpp"

namespace
{
	bool rayIntersectsTriangle(
		const spk::Vector2 &p_origin,
		const spk::Vector2 &p_direction,
		const spk::Vector2 &p_v0,
		const spk::Vector2 &p_v1,
		const spk::Vector2 &p_v2,
		float p_maxDistance,
		float &p_t)
	{
		spk::Vector3 origin = {p_origin.x, p_origin.y, 0.0f};
		spk::Vector3 dir = {p_direction.x, p_direction.y, 0.0f};
		spk::Vector3 v0 = {p_v0.x, p_v0.y, 0.0f};
		spk::Vector3 v1 = {p_v1.x, p_v1.y, 0.0f};
		spk::Vector3 v2 = {p_v2.x, p_v2.y, 0.0f};

		spk::Vector3 edge1 = v1 - v0;
		spk::Vector3 edge2 = v2 - v0;
		spk::Vector3 h = dir.cross(edge2);
		float a = edge1.dot(h);
		if ((a > -spk::Constants::pointPrecision && a < spk::Constants::pointPrecision) == true)
		{
			return false;
		}
		float f = 1.0f / a;
		spk::Vector3 s = origin - v0;
		float u = f * s.dot(h);
		if ((u < 0.0f || u > 1.0f) == true)
		{
			return false;
		}
		spk::Vector3 q = s.cross(edge1);
		float v = f * dir.dot(q);
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
		const spk::Vector2 &p_eye,
		const spk::Vector2 &p_dir,
		float p_t,
		const spk::SafePointer<spk::Entity> &p_owner,
		std::vector<spk::RayCast2D::Hit> &p_hits)
	{
		spk::RayCast2D::Hit hit{};
		hit.position = p_eye + p_dir * p_t;
		hit.entity = p_owner;
		p_hits.push_back(hit);
	}

	void processUnit(
		const spk::CollisionMesh2D::Unit &p_unit,
		const spk::Vector2 &p_eye,
		const spk::Vector2 &p_dir,
		float p_maxDistance,
		const spk::Vector2 &p_offset,
		const spk::SafePointer<spk::Entity> &p_owner,
		std::vector<spk::RayCast2D::Hit> &p_hits)
	{
		const auto &pts = p_unit.points();
		if ((pts.size() >= 3) == true)
		{
			spk::Vector2 base = pts[0] + p_offset;
			for (size_t i = 1; i + 1 < pts.size(); ++i)
			{
				float t = 0.0f;
				spk::Vector2 b = pts[i] + p_offset;
				spk::Vector2 c = pts[i + 1] + p_offset;
				if (rayIntersectsTriangle(p_eye, p_dir, base, b, c, p_maxDistance, t) == true)
				{
					addHit(p_eye, p_dir, t, p_owner, p_hits);
				}
			}
		}
	}

	void processCollider(
		const spk::SafePointer<const spk::CollisionMesh2D> &p_collisionMesh,
		const spk::Vector2 &p_eye,
		const spk::Vector2 &p_dir,
		float p_maxDistance,
		const spk::Vector2 &p_offset,
		const spk::SafePointer<spk::Entity> &p_owner,
		std::vector<spk::RayCast2D::Hit> &p_hits)
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

	void processBody(
		const spk::SafePointer<spk::RigidBody2D> &p_body,
		const spk::SafePointer<spk::GameEngine> &p_engine,
		const spk::Vector2 &p_eye,
		const spk::Vector2 &p_dir,
		float p_maxDistance,
		std::span<const std::wstring> p_tags,
		spk::BinaryOperator p_binaryOperator,
		std::vector<spk::RayCast2D::Hit> &p_hits)
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
		if ((p_tags.empty() == false))
		{
			if ((owner->containsTags(p_tags, p_binaryOperator) == false))
			{
				return;
			}
		}
		spk::Vector2 offset = owner->transform().position().xy();
		processCollider(p_body->collisionMesh(), p_eye, p_dir, p_maxDistance, offset, owner, p_hits);
	}
}

namespace spk
{
	namespace
	{
		std::vector<spk::RayCast2D::Hit> launchAllInternal(
			const spk::SafePointer<spk::GameEngine> &p_engine,
			const spk::Vector2 &p_eye,
			const spk::Vector2 &p_direction,
			float p_maxDistance,
			std::span<const std::wstring> p_tags,
			spk::BinaryOperator p_binaryOperator)
		{
			std::vector<spk::RayCast2D::Hit> hits;
			if ((p_engine == nullptr) == true)
			{
				return hits;
			}
			spk::Vector2 dir = p_direction.normalize();
			const std::vector<spk::SafePointer<spk::RigidBody2D>> &bodies = spk::RigidBody2D::getRigidBodies();
			for (const auto &body : bodies)
			{
				processBody(body, p_engine, p_eye, dir, p_maxDistance, p_tags, p_binaryOperator, hits);
			}
			return hits;
		}

		spk::RayCast2D::Hit launchInternal(
			const spk::SafePointer<spk::GameEngine> &p_engine,
			const spk::Vector2 &p_eye,
			const spk::Vector2 &p_direction,
			float p_maxDistance,
			std::span<const std::wstring> p_tags,
			spk::BinaryOperator p_binaryOperator)
		{
			std::vector<spk::RayCast2D::Hit> hits = launchAllInternal(p_engine, p_eye, p_direction, p_maxDistance, p_tags, p_binaryOperator);
			spk::RayCast2D::Hit result{};
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
	}

	RayCast2D::Hit RayCast2D::launch(
		const spk::SafePointer<spk::Entity> &p_cameraEntity,
		const spk::Vector2 &p_screenPosition,
		float p_maxDistance,
		std::span<const std::wstring> p_tags,
		spk::BinaryOperator p_binaryOperator)
	{
		if ((p_cameraEntity == nullptr) == true)
		{
			return {};
		}
		spk::SafePointer<spk::GameEngine> engine = p_cameraEntity->engine();
		spk::Vector2 eye = p_cameraEntity->transform().position().xy();
		spk::Vector2 world = screenPointToWorld(p_cameraEntity, p_screenPosition, 0.0f);
		spk::Vector2 dir = (world - eye).normalize();
		return launchInternal(engine, eye, dir, p_maxDistance, p_tags, p_binaryOperator);
	}

	std::vector<RayCast2D::Hit> RayCast2D::launchAll(
		const spk::SafePointer<spk::Entity> &p_cameraEntity,
		const spk::Vector2 &p_screenPosition,
		float p_maxDistance,
		std::span<const std::wstring> p_tags,
		spk::BinaryOperator p_binaryOperator)
	{
		if ((p_cameraEntity == nullptr) == true)
		{
			return {};
		}
		spk::SafePointer<spk::GameEngine> engine = p_cameraEntity->engine();
		spk::Vector2 eye = p_cameraEntity->transform().position().xy();
		spk::Vector2 world = screenPointToWorld(p_cameraEntity, p_screenPosition, 0.0f);
		spk::Vector2 dir = (world - eye).normalize();
		return launchAllInternal(engine, eye, dir, p_maxDistance, p_tags, p_binaryOperator);
	}

	spk::Vector2 RayCast2D::screenPointToWorld(
		const spk::SafePointer<spk::Entity> &p_cameraEntity, const spk::Vector2 &p_screenPosition, float p_layer)
	{
		if ((p_cameraEntity == nullptr) == true)
		{
			return {0, 0};
		}
		auto camComp = p_cameraEntity->getComponent<spk::CameraComponent>();
		if ((camComp == nullptr) == true)
		{
			return {0, 0};
		}
		spk::Vector3 cameraSpace = camComp->camera().convertScreenToCamera(spk::Vector3(p_screenPosition.x, p_screenPosition.y, p_layer));
		spk::Vector3 world = p_cameraEntity->transform().model() * cameraSpace;
		return world.xy();
	}
}
