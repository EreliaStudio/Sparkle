#pragma once

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

		virtual void _onUpdate(GameObject* p_gameObject) = 0;

	public:
		GameEngineModule();
		GameEngine* owner();
		const GameEngine* owner() const;
	};
}