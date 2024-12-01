#include "application/module/spk_mouse_module.hpp"

namespace spk
{
	void MouseModule::_treatEvent(spk::MouseEvent&& p_event)
	{
		switch (p_event.type)
		{
		case spk::MouseEvent::Type::Press:
		{
			if (p_event.button != spk::Mouse::Button::Unknow)
			{
				_mouse.buttons[static_cast<int>(p_event.button)] = spk::InputState::Down;
			}
			break;
		}
		case spk::MouseEvent::Type::Release:
		{
			if (p_event.button != spk::Mouse::Button::Unknow)
			{
				_mouse.buttons[static_cast<int>(p_event.button)] = spk::InputState::Up;
			}
			break;
		}
		case spk::MouseEvent::Type::DoubleClick:
		{

			break;
		}
		case spk::MouseEvent::Type::Motion:
		{
			_mouse.deltaPosition = p_event.position - _mouse.position;
			_mouse.position = p_event.position;
			break;
		}
		case spk::MouseEvent::Type::Wheel:
		{
			_mouse.wheel += p_event.scrollValue;
			break;
		}
		}

		p_event.mouse = &_mouse;
		_rootWidget->onMouseEvent(p_event);
	}

	spk::MouseEvent MouseModule::_convertEventToEventType(spk::Event&& p_event)
	{
		return (p_event.mouseEvent);
	}

	MouseModule::MouseModule()
	{

	}

	void MouseModule::linkToWidget(spk::Widget* p_rootWidget)
	{
		_rootWidget = p_rootWidget;
	}

	const spk::Mouse& MouseModule::mouse() const
	{
		return (_mouse);
	}
}