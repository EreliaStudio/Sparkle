#include "ui/widget/engine_widget.hpp"

#include <utility>

#include "core/context/update_context.hpp"

namespace spk
{
	EngineWidget::EngineWidget(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		setTargetRenderPass(Engine::SceneRenderPassKey);
	}

	void EngineWidget::setEngine(Engine *engine) noexcept
	{
		_engine = engine;
		if (_engine != nullptr)
		{
			_engine->handleGeometryChange(geometry());
		}
	}

	Engine *EngineWidget::engine() noexcept
	{
		return _engine;
	}

	const Engine *EngineWidget::engine() const noexcept
	{
		return _engine;
	}

	void EngineWidget::_onGeometryChange()
	{
		if (_engine != nullptr)
		{
			_engine->handleGeometryChange(geometry());
		}
	}

	void EngineWidget::_buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		if (_engine != nullptr)
		{
			_engine->buildRenderSnapshot(builder);
		}
	}

	void EngineWidget::_updateState(UpdateContext &context)
	{
		if (_engine != nullptr)
		{
			_engine->updateState(context);
		}
	}

	void EngineWidget::_onWindowResizedEvent(WindowResizedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onWindowMovedEvent(WindowMovedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onWindowFocusGainedEvent(WindowFocusGainedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onWindowFocusLostEvent(WindowFocusLostEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseEnteredEvent(MouseEnteredEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseLeftEvent(MouseLeftEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseMovedEvent(MouseMovedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseWheelScrolledEvent(MouseWheelScrolledEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseButtonDoubleClickedEvent(MouseButtonDoubleClickedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onKeyPressedEvent(KeyPressedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onKeyReleasedEvent(KeyReleasedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onTextInputEvent(TextInputEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->dispatch(event);
		}
	}

	void EngineWidget::_onPassiveMouseMovedEvent(MouseMovedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->observePointer(event);
		}
	}

	void EngineWidget::_onPassiveMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->observePointer(event);
		}
	}

	void EngineWidget::_onPassiveKeyPressedEvent(KeyPressedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->observeKeyboard(event);
		}
	}

	void EngineWidget::_onPassiveKeyReleasedEvent(KeyReleasedEvent &event)
	{
		if (_engine != nullptr)
		{
			_engine->observeKeyboard(event);
		}
	}
}
