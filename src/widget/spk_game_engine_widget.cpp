#include "widget/spk_game_engine_widget.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	void GameEngineWidget::_onPaintEvent(spk::PaintEvent& p_event)
	{
		if (_gameEngine == nullptr)
		{
			return ;
		}

		_fbo.activate();

		_gameEngine->onPaintEvent(p_event);

		_fbo.deactivate();
		
		_textureRenderer.render();
	}

	void GameEngineWidget::_onGeometryChange()
	{
		_fbo.resize(geometry().size);
		_textureRenderer.setTexture(_fbo.bindedTexture(L"outputColor"));
		_textureRenderer.prepare(geometry(), {{0.0f, 0.0f}, {1.0f, 1.0f}}, layer());
		_textureRenderer.validate();
	}

	void GameEngineWidget::_onUpdateEvent(spk::UpdateEvent& p_event)
	{
		if (_gameEngine == nullptr)
		{
			return ;
		}

		_gameEngine->onUpdateEvent(p_event);
	}

	void GameEngineWidget::_onMouseEvent(spk::MouseEvent& p_event)
	{
		if (_gameEngine == nullptr)
		{
			return ;
		}

		_gameEngine->onMouseEvent(p_event);
	}

	void GameEngineWidget::_onControllerEvent(spk::ControllerEvent& p_event)
	{
		if (_gameEngine == nullptr)
		{
			return ;
		}

		_gameEngine->onControllerEvent(p_event);
	}

	void GameEngineWidget::_onKeyboardEvent(spk::KeyboardEvent& p_event)
	{
		if (_gameEngine == nullptr)
		{
			return ;
		}

		_gameEngine->onKeyboardEvent(p_event);
	}

	void GameEngineWidget::_onTimerEvent(spk::TimerEvent& p_event)
	{
		if (_gameEngine == nullptr)
		{
			return ;
		}

		_gameEngine->onTimerEvent(p_event);
	}

	GameEngineWidget::GameEngineWidget(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_fbo.addAttachment(L"outputColor", 0, spk::OpenGL::FrameBufferObject::Type::Float4);
	}

	void GameEngineWidget::setGameEngine(spk::SafePointer<spk::GameEngine> p_gameEngine)
	{
		_gameEngine = p_gameEngine;
	}
}