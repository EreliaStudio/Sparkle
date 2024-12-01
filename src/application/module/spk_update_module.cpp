#include "application/module/spk_update_module.hpp"

#include "structure/graphics/spk_window.hpp"

namespace spk
{
	void UpdateModule::_treatEvent(spk::UpdateEvent&& p_event)
	{
		p_event.keyboard = _keyboard;
		p_event.mouse = _mouse;
		p_event.controller = _controller;
		_rootWidget->onUpdateEvent(p_event);
	}

	spk::UpdateEvent UpdateModule::_convertEventToEventType(spk::Event&& p_event)
	{
		return (p_event.updateEvent);
	}

	UpdateModule::UpdateModule()
	{

	}

	void UpdateModule::linkToMouse(const spk::Mouse* p_mouse)
	{
		_mouse = p_mouse;
	}

	void UpdateModule::linkToKeyboard(const spk::Keyboard* p_keyboard)
	{
		_keyboard = p_keyboard;
	}
	
	void UpdateModule::linkToController(const spk::Controller* p_controller)
	{
		_controller = p_controller;
	}

	void UpdateModule::linkToWidget(spk::Widget* p_rootWidget)
	{
		_rootWidget = p_rootWidget;
	}
}