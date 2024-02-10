#pragma once

#include "game_engine/component/spk_icollider_component.hpp"
#include "game_engine/spk_mesh.hpp"

namespace spk
{
	class SpriteCollider2D : public ICollider
	{
	private:
		spk::Vector3 _min;
		spk::Vector3 _max;

		spk::Vector3 _computeMin(const spk::Vector3& p_globalPosition, const spk::Vector3& p_globalScale, const spk::Mesh* p_mesh);
		spk::Vector3 _computeMax(const spk::Vector3& p_globalPosition, const spk::Vector3& p_globalScale, const spk::Mesh* p_mesh);

		void _onMeshEdition();

	public:
		SpriteCollider2D(const std::string& p_name);

		bool isIntersecting(const ICollider* p_otherCollider);
	};
}