#include "control_mapper.hpp"

#include "spk_debug_macro.hpp"

ControlMapper::ControlMapper(const std::wstring& p_name) :
	spk::Component(p_name)
{
	_controllerControls.push_back([&](const spk::ControllerEvent& p_event) -> Event {
				if (p_event.type == spk::ControllerEvent::Type::JoystickMotion &&
					p_event.joystick.id == spk::Controller::Joystick::Left && 
					std::abs(p_event.controller->leftJoystick.position.x) > std::abs(p_event.controller->leftJoystick.position.y) && 
					p_event.controller->leftJoystick.position.x < -100)
				{
					return (Event::PlayerMotionLeft);
				}
				return (Event::NoEvent);
			});
	_controllerControls.push_back([&](const spk::ControllerEvent& p_event) -> Event {
				if (p_event.type == spk::ControllerEvent::Type::JoystickMotion &&
					p_event.joystick.id == spk::Controller::Joystick::Left && 
					std::abs(p_event.controller->leftJoystick.position.x) > std::abs(p_event.controller->leftJoystick.position.y) && 
					p_event.controller->leftJoystick.position.x > 100)
				{
					return (Event::PlayerMotionRight);
				}
				return (Event::NoEvent);
			});
	_controllerControls.push_back([&](const spk::ControllerEvent& p_event) -> Event {
				if (p_event.type == spk::ControllerEvent::Type::JoystickMotion &&
					p_event.joystick.id == spk::Controller::Joystick::Left && 
					std::abs(p_event.controller->leftJoystick.position.y) > std::abs(p_event.controller->leftJoystick.position.x) && 
					p_event.controller->leftJoystick.position.y < -100)
				{
					return (Event::PlayerMotionDown);
				}
				return (Event::NoEvent);
			});
	_controllerControls.push_back([&](const spk::ControllerEvent& p_event) -> Event {
				if (p_event.type == spk::ControllerEvent::Type::JoystickMotion &&
					p_event.joystick.id == spk::Controller::Joystick::Left && 
					std::abs(p_event.controller->leftJoystick.position.y) > std::abs(p_event.controller->leftJoystick.position.x) && 
					p_event.controller->leftJoystick.position.y > 100)
				{
					return (Event::PlayerMotionUp);
				}
				return (Event::NoEvent);
			});
	_controllerControls.push_back([&](const spk::ControllerEvent& p_event) -> Event {
				if (p_event.type == spk::ControllerEvent::Type::JoystickReset &&
					p_event.joystick.id == spk::Controller::Joystick::Left)
				{	
					return (Event::PlayerMotionIdle);
				}
				return (Event::NoEvent);
			});

	_keyboardControls.push_back([&](const spk::KeyboardEvent& p_event) -> Event {
				if (p_event.type == spk::KeyboardEvent::Type::Press && (
					p_event.key == spk::Keyboard::Key::Q ||
					p_event.key == spk::Keyboard::Key::LeftArrow))
				{
					return (Event::PlayerMotionLeft);
				}
				return (Event::NoEvent);
			});
	_keyboardControls.push_back([&](const spk::KeyboardEvent& p_event) -> Event {
				if (p_event.type == spk::KeyboardEvent::Type::Press && (
					p_event.key == spk::Keyboard::Key::D ||
					p_event.key == spk::Keyboard::Key::RightArrow	))
				{
					return (Event::PlayerMotionRight);
				}
				return (Event::NoEvent);
			});
	_keyboardControls.push_back([&](const spk::KeyboardEvent& p_event) -> Event {
				if (p_event.type == spk::KeyboardEvent::Type::Press && (
						p_event.key == spk::Keyboard::Key::S ||
						p_event.key == spk::Keyboard::Key::DownArrow	))
				{
					return (Event::PlayerMotionDown);
				}
				return (Event::NoEvent);
			});
	_keyboardControls.push_back([&](const spk::KeyboardEvent& p_event) -> Event {
				if (p_event.type == spk::KeyboardEvent::Type::Press && (
					p_event.key == spk::Keyboard::Key::Z ||
					p_event.key == spk::Keyboard::Key::UpArrow	))
				{
					return (Event::PlayerMotionUp);
				}
				return (Event::NoEvent);
			});
	_keyboardControls.push_back([&](const spk::KeyboardEvent& p_event) -> Event {
				if (p_event.type == spk::KeyboardEvent::Type::Release)
				{
					if ((*(p_event.keyboard))[spk::Keyboard::Q] == spk::InputState::Down)
					{
						return (Event::PlayerMotionLeft);
					}
					if ((*(p_event.keyboard))[spk::Keyboard::D] == spk::InputState::Down)
					{
						return (Event::PlayerMotionRight);
					}
					if ((*(p_event.keyboard))[spk::Keyboard::S] == spk::InputState::Down)
					{
						return (Event::PlayerMotionDown);
					}
					if ((*(p_event.keyboard))[spk::Keyboard::Z] == spk::InputState::Down)
					{
						return (Event::PlayerMotionUp);
					}
					return (Event::PlayerMotionIdle);
				}
				return (Event::NoEvent);
			});
}

void ControlMapper::onControllerEvent(spk::ControllerEvent& p_event)
{
	for (const auto& control : _controllerControls)
	{
		Event event = control(p_event);
		if (event != Event::NoEvent)
		{
			EventCenter::instance()->notifyEvent(event);
		}
	}
}

void ControlMapper::onKeyboardEvent(spk::KeyboardEvent& p_event)
{
	for (const auto& control : _keyboardControls)
	{
		Event event = control(p_event);
		if (event != Event::NoEvent)
		{
			EventCenter::instance()->notifyEvent(event);
		}
	}
}

void ControlMapper::onMouseEvent(spk::MouseEvent& p_event)
{
	for (const auto& control : _mouseControls)
	{
		Event event = control(p_event);
		if (event != Event::NoEvent)
		{
			EventCenter::instance()->notifyEvent(event);
		}
	}
}

void ControlMapper::onUpdateEvent(spk::UpdateEvent& p_event)
{
	
}