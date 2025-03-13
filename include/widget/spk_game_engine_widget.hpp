#pragma once

#include "structure/spk_safe_pointer.hpp"
#include "widget/spk_widget.hpp"

#include "structure/engine/spk_game_engine.hpp"

#include "structure/graphics/opengl/spk_frame_buffer_object.hpp"

#include "structure/graphics/renderer/spk_texture_renderer.hpp"

namespace spk
{
	class GameEngineWidget : public spk::Widget
	{
	private:
		spk::SafePointer<spk::GameEngine> _gameEngine = nullptr;

		spk::OpenGL::FrameBufferObject _fbo;
		spk::TextureRenderer _textureRenderer;

		void _onGeometryChange();
		void _onPaintEvent(spk::PaintEvent &p_event);
		void _onUpdateEvent(spk::UpdateEvent &p_event);
		void _onMouseEvent(spk::MouseEvent &p_event);
		void _onControllerEvent(spk::ControllerEvent &p_event);
		void _onKeyboardEvent(spk::KeyboardEvent &p_event);
		void _onTimerEvent(spk::TimerEvent &p_event);

	public:
		GameEngineWidget(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent);

		void setGameEngine(spk::SafePointer<spk::GameEngine> p_gameEngine);
	};
}