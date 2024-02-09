#pragma once

#include <vector>

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
		std::vector<spk::GameObject*> _relevantObjects;

		virtual void _onUpdate() = 0;
		virtual bool _isObjectRelevant(spk::GameObject* p_gameObject) = 0;
		
		void _onGameObjectSubscription(spk::GameObject* p_gameObject);
		void _onGameObjectUnsubscription(spk::GameObject* p_gameObject);

	public:
		GameEngineModule();

		GameEngine* owner();
		const GameEngine* owner() const;

		std::vector<spk::GameObject*>& relevantObjects();
		const std::vector<spk::GameObject*>& relevantObjects() const;
	};
}