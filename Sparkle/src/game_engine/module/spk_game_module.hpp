#pragma once

namespace spk
{
	class GameEngine;
	class GameObject;

	class EngineModule
	{
		friend class GameEngine;

	private:
		static inline GameEngine* _creatingEngine = nullptr;
		GameEngine* _owner = nullptr;

		virtual void _onUpdate(GameObject* p_gameObject) = 0;

	public:
		EngineModule();
		GameEngine* owner();
		const GameEngine* owner() const;
	};
}