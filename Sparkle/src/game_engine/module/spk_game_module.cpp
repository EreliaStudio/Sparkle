#include "game_engine/module/spk_game_module.hpp"
#include "game_engine/spk_game_engine.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
	EngineModule::EngineModule()
	{
		_owner = _creatingEngine;
	}

	GameEngine* EngineModule::owner()
	{
		return (_owner);
	}

	const GameEngine* EngineModule::owner() const
	{
		return (_owner);
	}
}