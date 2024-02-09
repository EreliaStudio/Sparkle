#pragma once

#include "game_engine/module/spk_game_engine_module.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	class CollisionModule : public spk::GameEngineModule
	{
	private:
		void _onUpdate(GameObject* p_gameObject);
	
	public:
		CollisionModule();
	};
}