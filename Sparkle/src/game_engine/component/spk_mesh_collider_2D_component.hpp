#pragma once

#include "game_engine/component/spk_icollider_component.hpp"
#include "game_engine/spk_mesh.hpp"

namespace spk
{
	class MeshCollider2D : public ICollider
	{
	private:
		bool isIntersecting(const Mesh* p_mesh);

	public:
		MeshCollider2D(const std::string& p_name);
	};
}