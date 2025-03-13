#include "application/module/spk_controller_module.hpp"

#include "structure/graphics/spk_window.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	void ControllerModule::_treatEvent(spk::ControllerEvent &&p_event)
	{
		switch (p_event.type)
		{
		case spk::ControllerEvent::Type::JoystickMotion:
		{
			if (p_event.joystick.id == spk::Controller::Joystick::ID::Left)
			{
				_controller.leftJoystick.delta = p_event.joystick.values - _controller.leftJoystick.position;
				_controller.leftJoystick.position = p_event.joystick.values;

				if (std::abs(p_event.joystick.values.x) <= 20 && std::abs(p_event.joystick.values.y) <= 20)
				{
					ControllerInputThread::PostLeftJoystickReset(p_event.window->_hwnd);
				}
			}
			else if (p_event.joystick.id == spk::Controller::Joystick::ID::Right)
			{
				_controller.rightJoystick.delta = p_event.joystick.values - _controller.rightJoystick.position;
				_controller.rightJoystick.position = p_event.joystick.values;

				if (std::abs(p_event.joystick.values.x <= 20) && std::abs(p_event.joystick.values.y <= 20))
				{
					ControllerInputThread::PostRightJoystickReset(p_event.window->_hwnd);
				}
			}
			break;
		}
		case spk::ControllerEvent::Type::TriggerMotion:
		{
			if (p_event.trigger.id == spk::Controller::Trigger::ID::Left)
			{
				_controller.leftTrigger.ratio =
					static_cast<float>(p_event.trigger.value) / static_cast<float>(std::numeric_limits<unsigned short>::max());

				if (p_event.trigger.value > -100 && p_event.trigger.value < 100)
				{
					ControllerInputThread::PostLeftTriggerReset(p_event.window->_hwnd);
				}
			}
			else if (p_event.trigger.id == spk::Controller::Trigger::ID::Right)
			{
				_controller.rightTrigger.ratio =
					static_cast<float>(p_event.trigger.value) / static_cast<float>(std::numeric_limits<unsigned short>::max());

				if (p_event.trigger.value > -100 && p_event.trigger.value < 100)
				{
					ControllerInputThread::PostRightTriggerReset(p_event.window->_hwnd);
				}
			}
			break;
		}
		case spk::ControllerEvent::Type::DirectionalCrossMotion:
		{
			_controller.directionalCross = p_event.directionalCross.values;

			if (p_event.directionalCross.values.x <= 20 || p_event.directionalCross.values.y <= 20)
			{
				ControllerInputThread::PostDirectionalCrossReset(p_event.window->_hwnd);
			}
			break;
		}
		case spk::ControllerEvent::Type::Press:
		{
			if (p_event.button != spk::Controller::Button::Unknow)
			{
				_controller.buttons[static_cast<int>(p_event.button)] = spk::InputState::Down;
			}
			break;
		}
		case spk::ControllerEvent::Type::Release:
		{
			if (p_event.button != spk::Controller::Button::Unknow)
			{
				_controller.buttons[static_cast<int>(p_event.button)] = spk::InputState::Up;
			}
			break;
		}
		case spk::ControllerEvent::Type::JoystickReset:
		{
			if (p_event.joystick.id == spk::Controller::Joystick::ID::Left)
			{
				_controller.leftJoystick.delta = 0;
				_controller.leftJoystick.position = 0;
			}
			else if (p_event.joystick.id == spk::Controller::Joystick::ID::Right)
			{
				_controller.rightJoystick.delta = 0;
				_controller.rightJoystick.position = 0;
			}
			break;
		}
		case spk::ControllerEvent::Type::TriggerReset:
		{
			if (p_event.trigger.id == spk::Controller::Trigger::ID::Left)
			{
				_controller.leftTrigger.ratio = 0;
			}
			else if (p_event.trigger.id == spk::Controller::Trigger::ID::Right)
			{
				_controller.rightTrigger.ratio = 0;
			}
			break;
		}
		case spk::ControllerEvent::Type::DirectionalCrossReset:
		{
			_controller.directionalCross = 0;
			break;
		}
		default:
		{
			throw std::runtime_error("Unexpect event type.");
		}
		}
		p_event.controller = &_controller;

		if (spk::Widget::focusedWidget(Widget::FocusType::ControllerFocus) != nullptr)
		{
			spk::Widget::focusedWidget(Widget::FocusType::ControllerFocus)->onControllerEvent(p_event);
		}
		else
		{
			p_event.window->widget()->onControllerEvent(p_event);
		}
	}

	spk::ControllerEvent ControllerModule::_convertEventToEventType(spk::Event &&p_event)
	{
		return (p_event.controllerEvent);
	}

	ControllerModule::ControllerModule()
	{
	}

	const spk::Controller &ControllerModule::controller() const
	{
		return (_controller);
	}
}