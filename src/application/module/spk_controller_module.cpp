#include "application/module/spk_controller_module.hpp"

namespace spk
{
	void ControllerModule::_treatEvent(spk::ControllerEvent&& p_event)
	{
		switch (p_event.type)
		{
		case spk::ControllerEvent::Type::JoystickMotion:
		{
			if (p_event.joystick.id == spk::Controller::Joystick::ID::Left)
			{
				_controller.leftJoystick.delta = p_event.joystick.values - _controller.leftJoystick.position;
				_controller.leftJoystick.position = p_event.joystick.values;
			}
			else if (p_event.joystick.id == spk::Controller::Joystick::ID::Right)
			{
				_controller.rightJoystick.delta = p_event.joystick.values - _controller.rightJoystick.position;
				_controller.rightJoystick.position = p_event.joystick.values;
			}
			break;
		}
		case spk::ControllerEvent::Type::TriggerMotion:
		{
			if (p_event.trigger.id == spk::Controller::Trigger::ID::Left)
			{
				_controller.leftTrigger.ratio = static_cast<float>(p_event.trigger.value) / static_cast<float>(std::numeric_limits<unsigned short>::max());
			}
			else if (p_event.trigger.id == spk::Controller::Trigger::ID::Right)
			{
				_controller.rightTrigger.ratio = static_cast<float>(p_event.trigger.value) / static_cast<float>(std::numeric_limits<unsigned short>::max());
			}
			break;
		}
		case spk::ControllerEvent::Type::DirectionalCrossMotion:
		{
			_controller.directionalCross = p_event.directionalCross.values;
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
		}
		p_event.controller = &_controller;
		_rootWidget->onControllerEvent(p_event);
	}

	spk::ControllerEvent ControllerModule::_convertEventToEventType(spk::Event&& p_event)
	{
		return (p_event.controllerEvent);
	}

	ControllerModule::ControllerModule()
	{

	}

	void ControllerModule::linkToWidget(spk::Widget* p_rootWidget)
	{
		_rootWidget = p_rootWidget;
	}

	const spk::Controller& ControllerModule::controller() const
	{
		return (_controller);
	}
}