#include "application/module/spk_system_module.hpp"

#include "structure/graphics/spk_window.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	void SystemModule::_treatEvent(spk::SystemEvent p_event)
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
			p_event.window->requestResize(p_event.newSize);
			break;
		}
		case spk::SystemEvent::Type::ExitResize:
		{
			_isResizing = false;
			break;
		}
		case spk::SystemEvent::Type::TakeFocus:
		{
			//TODO(Hyarius): Manage focus taken
			break;
		}
		case spk::SystemEvent::Type::LoseFocus:
		{
			//TODO(Hyarius): Manage focus out
			break;
		}
		case spk::SystemEvent::Type::Move:
		{
			p_event.window->move(p_event.newPosition);
			break;
		}
		default:
		{
			GENERATE_ERROR("Unknow SystemEvent type received");
		}
		}
	}

	spk::SystemEvent SystemModule::_convertEventToEventType(spk::Event p_event)
	{
		return (p_event.systemEvent);
	}
}