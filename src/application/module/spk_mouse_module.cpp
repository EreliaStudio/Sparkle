#include "application/module/spk_mouse_module.hpp"

#include "structure/graphics/spk_window.hpp"

namespace spk
{
	void MouseModule::_treatEvent(spk::MouseEvent &&p_event)
	{
		_mouse._window = p_event.window;

		switch (p_event.type)
		{
		case spk::MouseEvent::Type::Press:
		{
			if (p_event.button != spk::Mouse::Button::Unknow)
			{
				_mouse._buttons[static_cast<int>(p_event.button)] = spk::InputState::Down;
			}
			break;
		}
		case spk::MouseEvent::Type::Release:
		{
			if (p_event.button != spk::Mouse::Button::Unknow)
			{
				_mouse._buttons[static_cast<int>(p_event.button)] = spk::InputState::Up;
			}
			break;
		}
		case spk::MouseEvent::Type::DoubleClick:
		{

			break;
		}
		case spk::MouseEvent::Type::Motion:
		{
			_mouse._deltaPosition = p_event.position - _mouse._position;
			_mouse._position = p_event.position;
			break;
		}
		case spk::MouseEvent::Type::Wheel:
		{
			_mouse._wheel += p_event.scrollValue;
			break;
		}
		default:
		{
			GENERATE_ERROR("Invalid MouseEvent type");
			break;
		}
		}

		p_event.mouse = &_mouse;

		if (spk::Widget::focusedWidget(Widget::FocusType::MouseFocus) != nullptr)
		{
			spk::Widget::focusedWidget(Widget::FocusType::MouseFocus)->onMouseEvent(p_event);
		}
		else
		{
			p_event.window->widget()->onMouseEvent(p_event);
		}
	}

	spk::MouseEvent MouseModule::_convertEventToEventType(spk::Event &&p_event)
	{
		return (p_event.mouseEvent);
	}

	MouseModule::MouseModule()
	{
	}

	const spk::Mouse &MouseModule::mouse() const
	{
		return (_mouse);
	}
}