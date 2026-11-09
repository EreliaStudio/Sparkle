#include "ui/widget/engine_widget.hpp"

#include <utility>

#include "core/context/update_context.hpp"

namespace spk
{
	EngineWidget::EngineWidget(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		setTargetRenderPass(RenderingEngine::SceneRenderPassKey);
	}

	void EngineWidget::setEngine(Engine *engine) noexcept
	{
		_engine = engine;
		_renderingEngine = dynamic_cast<RenderingEngine *>(engine);
		if (engine != nullptr)
		{
			engine->handleGeometryChange(geometry());
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
		if (engine() != nullptr)
		{
			engine()->handleGeometryChange(geometry());
		}
	}

	void EngineWidget::_buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->buildRenderSnapshot(builder);
		}
	}

	void EngineWidget::_updateState(UpdateContext &context)
	{
		if (engine() != nullptr)
		{
			engine()->updateState(context);
		}
	}

	void EngineWidget::_updateState(
		UpdateContext &context,
		DeviceContext &deviceContext)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->updateState(context, deviceContext);
		}
	}

	void EngineWidget::_onWindowResizedEvent(WindowResizedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onWindowMovedEvent(WindowMovedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onWindowFocusGainedEvent(WindowFocusGainedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onWindowFocusLostEvent(WindowFocusLostEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseEnteredEvent(MouseEnteredEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseLeftEvent(MouseLeftEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseMovedEvent(MouseMovedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseWheelScrolledEvent(MouseWheelScrolledEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onMouseButtonDoubleClickedEvent(
		MouseButtonDoubleClickedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onKeyPressedEvent(KeyPressedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onKeyReleasedEvent(KeyReleasedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onTextInputEvent(TextInputEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->dispatch(event);
		}
	}

	void EngineWidget::_onPassiveMouseMovedEvent(MouseMovedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->observePointer(event);
		}
	}

	void EngineWidget::_onPassiveMouseButtonPressedEvent(
		MouseButtonPressedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->observePointer(event);
		}
	}

	void EngineWidget::_onPassiveKeyPressedEvent(KeyPressedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->observeKeyboard(event);
		}
	}

	void EngineWidget::_onPassiveKeyReleasedEvent(KeyReleasedEvent &event)
	{
		if (_renderingEngine != nullptr)
		{
			_renderingEngine->observeKeyboard(event);
		}
	}
}
