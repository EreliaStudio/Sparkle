#include "application/module/spk_keyboard_module.hpp"

#include "structure/graphics/spk_window.hpp"

namespace spk
{
	void KeyboardModule::_treatEvent(spk::KeyboardEvent&& p_event)
	{
		switch (p_event.type)
		{
		case spk::KeyboardEvent::Type::Press:
		{
			if (p_event.key != spk::Keyboard::Key::Unknow)
			{
				if (_keyboard.state[static_cast<int>(p_event.key)] == spk::InputState::Down)
					return;
				_keyboard.state[static_cast<int>(p_event.key)] = spk::InputState::Down;
			}
			break;
		}
		case spk::KeyboardEvent::Type::Release:
		{
			if (p_event.key != spk::Keyboard::Key::Unknow)
			{
				if (_keyboard.state[static_cast<int>(p_event.key)] == spk::InputState::Up)
					return;
				_keyboard.state[static_cast<int>(p_event.key)] = spk::InputState::Up;
			}
			break;
		}
		case spk::KeyboardEvent::Type::Glyph:
		{
			_keyboard.glyph = p_event.glyph;
			break;
		}
		}
		p_event.keyboard = &_keyboard;

		if (spk::Widget::focusedWidget(Widget::FocusType::KeyboardFocus) != nullptr)
		{
			spk::Widget::focusedWidget(Widget::FocusType::KeyboardFocus)->onKeyboardEvent(p_event);
		}
		else
		{
			p_event.window->widget()->onKeyboardEvent(p_event);
		}
	}

	spk::KeyboardEvent KeyboardModule::_convertEventToEventType(spk::Event&& p_event)
	{
		return (p_event.keyboardEvent);
	}

	KeyboardModule::KeyboardModule()
	{

	}

	const spk::Keyboard& KeyboardModule::keyboard() const
	{
		return (_keyboard);
	}
}