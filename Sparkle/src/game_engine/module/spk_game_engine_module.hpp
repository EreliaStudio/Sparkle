#pragma once

#include <vector>
#include <string>
#include "profiler/spk_time_metric.hpp"

namespace spk
{
	class GameEngine;
	class GameObject;

	class GameEngineModule
	{
		friend class GameEngine;

	private:
		static inline GameEngine* _creatingEngine = nullptr;
		GameEngine* _owner = nullptr;
		std::string _name;
		std::vector<spk::GameObject*> _relevantObjects;

		TimeMetric& _timeMetric;

		virtual void _onUpdate() = 0;
		virtual bool _isObjectRelevant(spk::GameObject* p_gameObject) = 0;
		
		void _onGameObjectSubscription(spk::GameObject* p_gameObject);
		void _onGameObjectUnsubscription(spk::GameObject* p_gameObject);
		void update();

	public:
		GameEngineModule(const std::string& p_name);

		GameEngine* owner();
		const GameEngine* owner() const;

		std::vector<spk::GameObject*>& relevantObjects();
		const std::vector<spk::GameObject*>& relevantObjects() const;
	};
}