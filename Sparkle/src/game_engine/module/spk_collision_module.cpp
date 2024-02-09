#include "game_engine/module/spk_collision_module.hpp"
#include "game_engine/spk_game_object.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	void CollisionModule::_onUpdate(GameObject* p_gameObject)
	{
		if (spk::Application::activeApplication()->timeMetrics().deltaTime() != 0)
		{
			
		}
	}
	
	CollisionModule::CollisionModule()
	{

	}
}