#include "game_engine/component/spk_sprite_collider_2D_component.hpp"
#include "game_engine/spk_game_object.hpp"
#include "game_engine/component/spk_physics_component.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	spk::Vector3 SpriteCollider2D::_computeMin(const spk::Vector3& p_globalPosition, const spk::Vector3& p_globalScale, const spk::Mesh* p_mesh)
	{
		if (p_mesh == nullptr)
			return (spk::Vector3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));

		spk::Vector3 result = p_mesh->points()[p_mesh->indexes()[0]] * p_globalScale + p_globalPosition;
		
		for (size_t i = 1; i < p_mesh->indexes().size(); i += 3)
		{
			result = spk::Vector3::min(result, p_mesh->points()[p_mesh->indexes()[i]] * p_globalScale + p_globalPosition);
		}

		return (result);
	}

	spk::Vector3 SpriteCollider2D::_computeMax(const spk::Vector3& p_globalPosition, const spk::Vector3& p_globalScale, const spk::Mesh* p_mesh)
	{
		if (p_mesh == nullptr)
			return (spk::Vector3(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()));
			
		spk::Vector3 result = p_mesh->points()[p_mesh->indexes()[0]] * p_globalScale + p_globalPosition;
		
		for (size_t i = 1; i < p_mesh->indexes().size(); i += 3)
		{
			result = spk::Vector3::max(result, p_mesh->points()[p_mesh->indexes()[i]] * p_globalScale + p_globalPosition);
		}

		return (result);
	}

	bool SpriteCollider2D::isIntersecting(const ICollider* p_otherCollider)
	{
		const SpriteCollider2D* castedPointer = dynamic_cast<const SpriteCollider2D*>(p_otherCollider);

		auto* physicsComponentA = owner()->getComponent<spk::Physics>();
		auto* physicsComponentB = p_otherCollider->owner()->getComponent<spk::Physics>();

		spk::Vector3 deltaPositionA = physicsComponentA ? physicsComponentA->nextVelocityTick() : spk::Vector3(0, 0, 0);
		spk::Vector3 deltaPositionB = physicsComponentB ? physicsComponentB->nextVelocityTick() : spk::Vector3(0, 0, 0);

		spk::Vector3 minA = _min + deltaPositionA;
		spk::Vector3 maxA = _max + deltaPositionA;

		spk::Vector3 minB, maxB;
		if (castedPointer != nullptr)
		{
			minB = castedPointer->_min + deltaPositionB;
			maxB = castedPointer->_max + deltaPositionB;
		}
		else
		{
			minB = _computeMin(p_otherCollider->owner()->globalPosition(), p_otherCollider->owner()->globalScale(), p_otherCollider->mesh()) + deltaPositionB;
			maxB = _computeMax(p_otherCollider->owner()->globalPosition(), p_otherCollider->owner()->globalScale(), p_otherCollider->mesh()) + deltaPositionB;
		}

		bool isNotIntersecting = maxA.x < minB.x || minA.x > maxB.x ||
								maxA.y < minB.y || minA.y > maxB.y ||
								maxA.z < minB.z || minA.z > maxB.z;

		return !isNotIntersecting;
	}

	void SpriteCollider2D::_onMeshEdition()
	{
		_min = _computeMin(owner()->globalPosition(), owner()->globalScale(), mesh());
		_max = _computeMax(owner()->globalPosition(), owner()->globalScale(), mesh());
	}

	SpriteCollider2D::SpriteCollider2D(const std::string& p_name) :
		ICollider(p_name)
	{

	}
}