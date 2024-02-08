#pragma once

#include "game_engine/component/spk_game_component.hpp"

namespace spk
{
	class Physics : public GameComponent
	{
	private:
		void _onRender();
		void _onUpdate();

	public:
		Physics(const std::string& p_name);
	};
}