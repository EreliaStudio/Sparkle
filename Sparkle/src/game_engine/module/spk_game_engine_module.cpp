#include "game_engine/module/spk_game_engine_module.hpp"
#include "game_engine/spk_game_engine.hpp"
#include "game_engine/spk_game_object.hpp"

namespace spk
{
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
}