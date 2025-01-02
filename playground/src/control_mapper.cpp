#include "control_mapper.hpp"

ControlMapper::ControlMapper(const std::wstring& p_name) :
		spk::Component(p_name)
	{

	}

	void ControlMapper::onControllerEvent(spk::ControllerEvent& p_event)
	{
		if (p_event.type == spk::ControllerEvent::Type::JoystickMotion)
		{
			if (p_event.joystick.id == spk::Controller::Joystick::Right)
				return ;
			_motionEvent = Event::NoEvent;

			if (std::abs(p_event.controller->leftJoystick.position.x) > std::abs(p_event.controller->leftJoystick.position.y))
			{
				if (p_event.controller->leftJoystick.position.x < -100)
				{
					_motionEvent = Event::PlayerMotionLeft;
				}
				else if (p_event.controller->leftJoystick.position.x > 100)
				{
					_motionEvent = Event::PlayerMotionRight;
				}
			}
			else
			{
				if (p_event.controller->leftJoystick.position.y < -100)
				{
					_motionEvent = Event::PlayerMotionDown;
				}
				else if (p_event.controller->leftJoystick.position.y > 100)
				{
					_motionEvent = Event::PlayerMotionUp;
				}
			}
		}
		else if (p_event.type == spk::ControllerEvent::Type::JoystickReset)
		{
			if (p_event.joystick.id == spk::Controller::Joystick::Right)
				return ;
				
			_motionEvent = Event::PlayerMotionIdle;
		}
	}

	void ControlMapper::onKeyboardEvent(spk::KeyboardEvent& p_event)
	{
		if (p_event.type == spk::KeyboardEvent::Type::Press ||
			p_event.type == spk::KeyboardEvent::Type::Release)
		{
			_motionEvent = Event::PlayerMotionIdle;

			if (p_event.keyboard->state[static_cast<int>(spk::Keyboard::Z)] == spk::InputState::Down)
			{
				_motionEvent = Event::PlayerMotionUp;
			}
			else if (p_event.keyboard->state[static_cast<int>(spk::Keyboard::Q)] == spk::InputState::Down)
			{
				_motionEvent = Event::PlayerMotionLeft;
			}
			else if (p_event.keyboard->state[static_cast<int>(spk::Keyboard::S)] == spk::InputState::Down)
			{
				_motionEvent = Event::PlayerMotionDown;
			}
			else if (p_event.keyboard->state[static_cast<int>(spk::Keyboard::D)] == spk::InputState::Down)
			{
				_motionEvent = Event::PlayerMotionRight;
			}
		}
	}

	void ControlMapper::onMouseEvent(spk::MouseEvent& p_event)
	{

	}

	void ControlMapper::onUpdateEvent(spk::UpdateEvent& p_event)
	{
		static Event _lastEvent = Event::NoEvent;

		if (_motionEvent != _lastEvent)
		{
			EventCenter::instance()->notifyEvent(_motionEvent);
			_lastEvent = _motionEvent;
		}
	}