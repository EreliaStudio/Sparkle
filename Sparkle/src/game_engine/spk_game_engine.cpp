#include "game_engine/spk_game_engine.hpp"

namespace spk
{
	GameEngine::GameEngine()
	{

	}

	void GameEngine::render()
	{
		for (auto& gameObject : _subscribedObjects)
		{
			gameObject->render();
		}
	}

	void GameEngine::update()
	{
		for (auto& gameObject : _subscribedObjects)
		{
			gameObject->update();
		}
	}

	void GameEngine::subscribe(GameObject* p_newObject)
	{
		if (std::find(_subscribedObjects.begin(), _subscribedObjects.end(), p_newObject) == _subscribedObjects.end())
		{
			_subscribedObjects.push_back(p_newObject);
		}
	}

	void GameEngine::unsubscribe(GameObject* p_newObject)
	{
		_subscribedObjects.erase(
			std::remove(_subscribedObjects.begin(), _subscribedObjects.end(), p_newObject), 
			_subscribedObjects.end());
	}

	GameObject* GameEngine::getObject(const Comparator& p_comparator)
	{
		for (auto& object : _subscribedObjects)
		{
			if (p_comparator(object) == true)
			{
				return (object);
			}
		}
		return (nullptr);
	}

	std::vector<GameObject*> GameEngine::getObjectList(const Comparator& p_comparator)
	{
		std::vector<GameObject*> result;

		for (auto& object : _subscribedObjects)
		{
			if (p_comparator(object) == true)
			{
				result.push_back(object);
			}
		}

		return (result);
	}
	
	GameObject* GameEngine::getObject(const std::string& p_name)
	{
		return (getObject([p_name](GameObject* p_object) -> bool {return (p_object->name() == p_name);}));
	}

	std::vector<GameObject*> GameEngine::getObjectList(const std::string& p_name)
	{
		return (getObjectList([p_name](GameObject* p_object) -> bool {return (p_object->name() == p_name);}));
	}
}