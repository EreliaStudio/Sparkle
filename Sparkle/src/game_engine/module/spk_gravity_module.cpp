#include "game_engine/module/spk_gravity_module.hpp"
#include "game_engine/component/spk_physics_component.hpp"
#include "game_engine/spk_game_object.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	void GravityModule::_onUpdate(GameObject* p_gameObject)
	{
		if (spk::Application::activeApplication()->timeMetrics().deltaTime() != 0)
		{
			spk::Physics* physicsComponent = p_gameObject->getComponent<spk::Physics>();

			if (physicsComponent != nullptr && physicsComponent->isKinematic() == true)
			{
				physicsComponent->applyAcceleration(_gravityAcceleration);
			}
		}
	}
	
	GravityModule::GravityModule() :
		_gravityAcceleration(0.0f, 0.0f, 0.0f)
	{

	}

	GravityModule::GravityModule(const spk::Vector3& p_gravityAcceleration) :
		_gravityAcceleration(p_gravityAcceleration)
	{

	}
}