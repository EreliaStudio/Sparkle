#include "game_engine/module/spk_game_engine_module.hpp"
#include "game_engine/spk_game_engine.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	void GameEngineModule::_onGameObjectSubscription(spk::GameObject* p_gameObject)
	{
		if (std::find(_relevantObjects.begin(), _relevantObjects.end(), p_gameObject) == _relevantObjects.end())
		{
			if (_isObjectRelevant(p_gameObject) == true)
			{
				_relevantObjects.push_back(p_gameObject);
			}
		}
	}
	
	void GameEngineModule::_onGameObjectUnsubscription(spk::GameObject* p_gameObject)
	{
		_relevantObjects.erase(
			std::remove(_relevantObjects.begin(), _relevantObjects.end(), p_gameObject), 
			_relevantObjects.end());
	}

	GameEngineModule::GameEngineModule()
	{
		_owner = _creatingEngine;
	}

	GameEngine* GameEngineModule::owner()
	{
		return (_owner);
	}

	const GameEngine* GameEngineModule::owner() const
	{
		return (_owner);
	}

	std::vector<spk::GameObject*>& GameEngineModule::relevantObjects()
	{
		return (_relevantObjects);
	}
	
	const std::vector<spk::GameObject*>& GameEngineModule::relevantObjects() const
	{
		return (_relevantObjects);
	}
}