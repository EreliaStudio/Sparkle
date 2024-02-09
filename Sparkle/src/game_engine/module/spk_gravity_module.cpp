#include "game_engine/module/spk_gravity_module.hpp"
#include "game_engine/component/spk_physics_component.hpp"
#include "game_engine/spk_game_object.hpp"
#include "game_engine/spk_game_engine.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	void GravityModule::_applyGravityAcceleration(GameObject* p_gameObject)
	{
		spk::Physics* physicsComponent = p_gameObject->getComponent<spk::Physics>();

		if (physicsComponent != nullptr && physicsComponent->isKinematic() == true)
		{
			physicsComponent->applyAcceleration(_gravityAcceleration);
		}
	}
	
	void GravityModule::_onUpdate()
	{
		if (spk::Application::activeApplication()->timeMetrics().deltaTime() != 0)
		{
			for (auto& object : owner()->subscribedGameObjects())
			{
				_applyGravityAcceleration(object);
			}
		}
	}

	bool GravityModule::_isObjectRelevant(spk::GameObject* p_gameObject)
	{
		return (true);
	}

	GravityModule::GravityModule() :
		GameEngineModule(),
		_gravityAcceleration(0.0f, 0.0f, 0.0f)
	{

	}

	GravityModule::GravityModule(const spk::Vector3& p_gravityAcceleration) :
		_gravityAcceleration(p_gravityAcceleration)
	{

	}
}