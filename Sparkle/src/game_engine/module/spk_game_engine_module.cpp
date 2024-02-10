#include "game_engine/module/spk_game_engine_module.hpp"
#include "game_engine/spk_game_engine.hpp"
#include "game_engine/spk_game_object.hpp"
#include "application/spk_application.hpp"

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

	void GameEngineModule::update()
	{
		_timeMetric.start();
		_onUpdate();
		_timeMetric.stop();
	}
	
	void GameEngineModule::_onGameObjectUnsubscription(spk::GameObject* p_gameObject)
	{
		_relevantObjects.erase(
			std::remove(_relevantObjects.begin(), _relevantObjects.end(), p_gameObject), 
			_relevantObjects.end());
	}

	GameEngineModule::GameEngineModule(const std::string& p_name) :
		_name(p_name),
		_timeMetric(spk::Application::activeApplication()->profiler().metric<TimeMetric>("Module : " + _name))
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