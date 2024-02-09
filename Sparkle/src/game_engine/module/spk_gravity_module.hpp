#pragma once

#include "game_engine/module/spk_game_engine_module.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	class GravityModule : public spk::GameEngineModule
	{
	private:
		spk::Vector3 _gravityAcceleration;

		void _applyGravityAcceleration(GameObject* p_gameObject);
		void _onUpdate();
		bool _isObjectRelevant(spk::GameObject* p_gameObject);
	
	public:
		GravityModule();
		GravityModule(const spk::Vector3& p_gravityAcceleration);
	};
}