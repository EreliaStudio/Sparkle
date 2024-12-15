#pragma once

#include "structure/spk_safe_pointer.hpp"
#include "widget/spk_widget.hpp"

#include "structure/engine/spk_game_engine.hpp"

namespace spk
{
	class GameEngineWidget : public spk::Widget
	{
	private:
		spk::SafePointer<spk::GameEngine> _gameEngine = nullptr;

		void onPaintEvent(spk::PaintEvent& p_event);
		void onUpdateEvent(spk::UpdateEvent& p_event);
		void onMouseEvent(spk::MouseEvent& p_event);
		void onControllerEvent(spk::ControllerEvent& p_event);
		void onKeyboardEvent(spk::KeyboardEvent& p_event);
		void onTimerEvent(spk::TimerEvent& p_event);

	public:
		GameEngineWidget(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);

		void setGameEngine(spk::SafePointer<spk::GameEngine> p_gameEngine);
	};
}