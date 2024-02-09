#include "game_engine/component/spk_sprite_collider_2D_component.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	spk::Vector3 SpriteCollider2D::_computeMin(const spk::Vector3& p_globalPosition, const spk::Mesh* p_mesh)
	{
		if (p_mesh == nullptr)
			return (spk::Vector3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));

		spk::Vector3 result = p_mesh->points()[p_mesh->indexes()[0]] + p_globalPosition;
		
		for (size_t i = 1; i < p_mesh->indexes().size(); i += 3)
		{
			result = spk::Vector3::min(result, p_mesh->points()[p_mesh->indexes()[i]] + p_globalPosition);
		}

		return (result);
	}

	spk::Vector3 SpriteCollider2D::_computeMax(const spk::Vector3& p_globalPosition, const spk::Mesh* p_mesh)
	{
		if (p_mesh == nullptr)
			return (spk::Vector3(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()));
			
		spk::Vector3 result = p_mesh->points()[p_mesh->indexes()[0]] + p_globalPosition;
		
		for (size_t i = 1; i < p_mesh->indexes().size(); i += 3)
		{
			result = spk::Vector3::max(result, p_mesh->points()[p_mesh->indexes()[i]] + p_globalPosition);
		}

		return (result);
	}

	bool SpriteCollider2D::isIntersecting(const ICollider* p_otherCollider)
	{
		const SpriteCollider2D* castedPointer = dynamic_cast<const SpriteCollider2D*>(p_otherCollider);

		if (castedPointer != nullptr)
		{
			if (spk::Vector3::isInsideRectangle(castedPointer->_min, _min, _max) == true ||
				spk::Vector3::isInsideRectangle(castedPointer->_max, _min, _max) == true ||
				spk::Vector3::isInsideRectangle(_min, castedPointer->_min, castedPointer->_max) == true ||
				spk::Vector3::isInsideRectangle(_max, castedPointer->_min, castedPointer->_max) == true)
				return (true);
			return (false);
		}
		else
		{
			spk::Vector3 minB = _computeMin(p_otherCollider->owner()->globalPosition(), p_otherCollider->mesh());
			spk::Vector3 maxB = _computeMax(p_otherCollider->owner()->globalPosition(), p_otherCollider->mesh());

			if (spk::Vector3::isInsideRectangle(minB, _min, _max) == true ||
				spk::Vector3::isInsideRectangle(maxB, _min, _max) == true ||
				spk::Vector3::isInsideRectangle(_min, minB, maxB) == true ||
				spk::Vector3::isInsideRectangle(_max, minB, maxB) == true)
				return (true);
			return (false);
		}
	}

	void SpriteCollider2D::_onMeshEdition()
	{
		_min = _computeMin(owner()->globalPosition(), mesh());
		_max = _computeMax(owner()->globalPosition(), mesh());
	}

	SpriteCollider2D::SpriteCollider2D(const std::string& p_name) :
		ICollider(p_name)
	{

	}
}