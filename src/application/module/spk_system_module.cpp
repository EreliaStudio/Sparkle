#include "application/module/spk_system_module.hpp"

#include "application/spk_graphical_application.hpp"

namespace spk
{
	void SystemModule::_treatEvent(spk::SystemEvent&& p_event)
	{
		/* Event types :
				Resize,
				TakeFocus,
				LoseFocus,
				Quit,
				Move
		*/
		switch (p_event.type)
		{
		case spk::SystemEvent::Type::Quit:
		{
			p_event.window->close();
			break;
		}
		case spk::SystemEvent::Type::EnterResize:
		{

			break;
		}
		case spk::SystemEvent::Type::Resize:
		{
			p_event.window->resize(p_event.newSize);
			p_event.window->requestPaint();
			break;
		}
		case spk::SystemEvent::Type::ExitResize:
		{
			p_event.window->requestPaint();
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

		}
	}

	spk::SystemEvent SystemModule::_convertEventToEventType(spk::Event&& p_event)
	{
		return (p_event.systemEvent);
	}

	SystemModule::SystemModule()
	{

	}
}