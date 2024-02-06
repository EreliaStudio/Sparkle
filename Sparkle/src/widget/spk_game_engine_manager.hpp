#pragma once

#include "widget/spk_widget.hpp"
#include "game_engine/spk_game_engine.hpp"

namespace spk
{
	class GameEngineManager : public spk::IWidget
	{
	private:
		spk::GameEngine* _engine = nullptr;

		void _onGeometryChange();
		void _onRender();
		void _onUpdate();

	public:
		GameEngineManager(const std::string& p_name);
		GameEngineManager(const std::string& p_name, spk::IWidget* p_parent);

		void setGameEngine(spk::GameEngine* p_engine);
	};
}