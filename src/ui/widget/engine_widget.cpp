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
		_renderingEngine.setEngine(engine);
		if (engine != nullptr)
		{
			engine->handleGeometryChange(geometry());
		}
	}

	Engine *EngineWidget::engine() noexcept
	{
		return _renderingEngine.engine();
	}

	const Engine *EngineWidget::engine() const noexcept
	{
		return _renderingEngine.engine();
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
		_renderingEngine.buildRenderSnapshot(builder);
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
		_renderingEngine.updateState(context, deviceContext);
	}

	void EngineWidget::_onWindowResizedEvent(WindowResizedEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onWindowMovedEvent(WindowMovedEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onWindowFocusGainedEvent(WindowFocusGainedEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onWindowFocusLostEvent(WindowFocusLostEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onMouseEnteredEvent(MouseEnteredEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onMouseLeftEvent(MouseLeftEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onMouseMovedEvent(MouseMovedEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onMouseWheelScrolledEvent(MouseWheelScrolledEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onMouseButtonPressedEvent(MouseButtonPressedEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onMouseButtonReleasedEvent(MouseButtonReleasedEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onMouseButtonDoubleClickedEvent(
		MouseButtonDoubleClickedEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onKeyPressedEvent(KeyPressedEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onKeyReleasedEvent(KeyReleasedEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onTextInputEvent(TextInputEvent &event)
	{
		_renderingEngine.dispatch(event);
	}

	void EngineWidget::_onPassiveMouseMovedEvent(MouseMovedEvent &event)
	{
		_renderingEngine.observePointer(event);
	}

	void EngineWidget::_onPassiveMouseButtonPressedEvent(
		MouseButtonPressedEvent &event)
	{
		_renderingEngine.observePointer(event);
	}

	void EngineWidget::_onPassiveKeyPressedEvent(KeyPressedEvent &event)
	{
		_renderingEngine.observeKeyboard(event);
	}

	void EngineWidget::_onPassiveKeyReleasedEvent(KeyReleasedEvent &event)
	{
		_renderingEngine.observeKeyboard(event);
	}
}
