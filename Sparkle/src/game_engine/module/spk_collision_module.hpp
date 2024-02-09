#pragma once

#include "game_engine/module/spk_game_engine_module.hpp"
#include "math/spk_vector3.hpp"

namespace spk
{
	class CollisionModule : public spk::GameEngineModule
	{
	private:
		struct Hit
		{
			spk::GameObject* a;
			spk::GameObject* b;
		};

		void _treatCollision(const Hit& p_hit);
		bool _testCollisionBetweenObject(spk::GameObject* p_gameObjectA, spk::GameObject* p_gameObjectB);
		void _onUpdate();
		bool _isObjectRelevant(spk::GameObject* p_gameObject);
	
	public:
		CollisionModule();
	};
}