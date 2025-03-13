#include "application/module/spk_system_module.hpp"

#include "structure/graphics/spk_window.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	void SystemModule::_treatEvent(spk::SystemEvent &&p_event)
	{
		switch (p_event.type)
		{
		case spk::SystemEvent::Type::Quit:
		{
			p_event.window->close();
			break;
		}
		case spk::SystemEvent::Type::EnterResize:
		{
			_isResizing = true;

			break;
		}
		case spk::SystemEvent::Type::Resize:
		{
			p_event.window->resize(p_event.newSize);

			if (_isResizing == false)
			{
				p_event.window->requestResize();
			}
			break;
		}
		case spk::SystemEvent::Type::ExitResize:
		{
			_isResizing = false;
			p_event.window->requestResize();
			break;
		}
		case spk::SystemEvent::Type::TakeFocus:
		{

			break;
		}
		case spk::SystemEvent::Type::LoseFocus:
		{

			break;
		}
		case spk::SystemEvent::Type::Move:
		{
			p_event.window->move(p_event.newPosition);
			break;
		}
		case spk::SystemEvent::Type::SetCursor:
		{
			if (p_event.window->_savedCursor != p_event.window->_currentCursor)
			{
				p_event.window->_applyCursor();
			}
			break;
		}
		}
	}

	spk::SystemEvent SystemModule::_convertEventToEventType(spk::Event &&p_event)
	{
		return (p_event.systemEvent);
	}

	SystemModule::SystemModule()
	{
	}
}