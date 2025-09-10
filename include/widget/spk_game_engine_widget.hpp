#pragma once

#include "structure/spk_safe_pointer.hpp"
#include "widget/spk_widget.hpp"

#include "structure/engine/spk_game_engine.hpp"

#include "structure/graphics/opengl/spk_frame_buffer_object.hpp"

#include "structure/graphics/painter/spk_texture_painter.hpp"

namespace spk
{
	class GameEngineWidget : public spk::Widget
	{
	private:
		spk::SafePointer<spk::GameEngine> _gameEngine = nullptr;

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent &p_event) override;
		void _onUpdateEvent(spk::UpdateEvent &p_event) override;
		void _onMouseEvent(spk::MouseEvent &p_event) override;
		void _onControllerEvent(spk::ControllerEvent &p_event) override;
		void _onKeyboardEvent(spk::KeyboardEvent &p_event) override;
		void _onTimerEvent(spk::TimerEvent &p_event) override;

	public:
		GameEngineWidget(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent);

		void setGameEngine(spk::SafePointer<spk::GameEngine> p_gameEngine);
	};
}