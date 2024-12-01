#include "application/module/spk_keyboard_module.hpp"

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
		_rootWidget->onKeyboardEvent(p_event);
	}

	spk::KeyboardEvent KeyboardModule::_convertEventToEventType(spk::Event&& p_event)
	{
		return (p_event.keyboardEvent);
	}

	KeyboardModule::KeyboardModule() : 
		_rootWidget(nullptr)
	{

	}

	void KeyboardModule::linkToWidget(spk::Widget* p_rootWidget)
	{
		_rootWidget = p_rootWidget;
	}

	const spk::Keyboard& KeyboardModule::keyboard() const
	{
		return (_keyboard);
	}
}