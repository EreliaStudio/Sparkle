#include "game_engine/module/spk_collision_module.hpp"
#include "game_engine/spk_game_object.hpp"
#include "game_engine/spk_game_engine.hpp"
#include "application/spk_application.hpp"
#include "game_engine/component/spk_icollider_component.hpp"
#include "game_engine/component/spk_physics_component.hpp"

namespace spk
{	
	bool CollisionModule::_testCollisionBetweenObject(spk::GameObject* p_gameObjectA, spk::GameObject* p_gameObjectB)
	{
		spk::ICollider* colliderA = p_gameObjectA->getComponent<spk::ICollider>();
		spk::ICollider* colliderB = p_gameObjectB->getComponent<spk::ICollider>();

		return (colliderA->isIntersecting(colliderB) == true || colliderB->isIntersecting(colliderA) == true);
	}

	void CollisionModule::_treatCollision(const CollisionModule::Hit& p_hit)
	{
		std::cout << "Collision between " << p_hit.a->name() << "(" << p_hit.a->getComponent<spk::Physics>()->velocity() << ") and " << p_hit.b->name() << std::endl;
	}

	void CollisionModule::_onUpdate()
	{
		std::vector<Hit> hits;
		std::vector<spk::GameObject*> objectToCheck = relevantObjects();
		for (size_t i = 0; i < objectToCheck.size(); i++)
		{
			spk::Physics* physicsComponent = objectToCheck[i]->getComponent<spk::Physics>();
			if (physicsComponent != nullptr && physicsComponent->velocity() != 0)
			{
				for (size_t j = i + 1; j < objectToCheck.size(); j++)
				{
					if (_testCollisionBetweenObject(objectToCheck[i], objectToCheck[j]) == true)
					{
						hits.push_back(Hit(objectToCheck[i], objectToCheck[j]));
					}
				}
			}
		}

		for (auto& hit : hits)
		{
			_treatCollision(hit);
		}
	}

	bool CollisionModule::_isObjectRelevant(spk::GameObject* p_gameObject)
	{
		if (p_gameObject->getComponent<spk::ICollider>() != nullptr)
			return (true);
		return (false);
	}

	CollisionModule::CollisionModule() :
		GameEngineModule("CollisionModule")
	{

	}
}