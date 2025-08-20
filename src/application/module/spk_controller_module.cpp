#include "application/module/spk_controller_module.hpp"

#include "spk_debug_macro.hpp"
#include "structure/graphics/spk_window.hpp"
#include "structure/system/spk_exception.hpp"

namespace spk
{
	static constexpr int kJoystickDeadzone = 20;
	static constexpr int kTriggerDeadzone = 100;

	bool ControllerModule::_inDeadzone2D(const spk::Vector2Int &p_v, int p_dz) noexcept
	{
		return (std::abs(p_v.x) <= p_dz) && (std::abs(p_v.y) <= p_dz);
	}

	bool ControllerModule::_nearZero(int p_v, int p_dz) noexcept
	{
		return (-p_dz <= p_v) && (p_v <= p_dz);
	}

	void ControllerModule::_handleJoystickMotion(const spk::ControllerEvent &p_e)
	{
		_controller._window = p_e.window;

		switch (p_e.joystick.id)
		{
		case spk::Controller::Joystick::ID::Left:
		{
			_controller._leftJoystick.delta = p_e.joystick.values - _controller._leftJoystick.position;
			_controller._leftJoystick.position = p_e.joystick.values;

			if (_inDeadzone2D(p_e.joystick.values, kJoystickDeadzone))
			{
				ControllerInputThread::PostLeftJoystickReset(p_e.window->_hwnd);
			}
			return;
		}
		case spk::Controller::Joystick::ID::Right:
		{
			_controller._rightJoystick.delta = p_e.joystick.values - _controller._rightJoystick.position;
			_controller._rightJoystick.position = p_e.joystick.values;

			if (_inDeadzone2D(p_e.joystick.values, kJoystickDeadzone))
			{
				ControllerInputThread::PostRightJoystickReset(p_e.window->_hwnd);
			}
			return;
		}
		default:
			GENERATE_ERROR("Unexpected joystick id.");
		}
	}

	void ControllerModule::_handleTriggerMotion(const spk::ControllerEvent &p_e)
	{
		_controller._window = p_e.window;

		const float ratio = static_cast<float>(p_e.trigger.value) / static_cast<float>(std::numeric_limits<unsigned short>::max());

		switch (p_e.trigger.id)
		{
		case spk::Controller::Trigger::ID::Left:
			_controller._leftTrigger.ratio = ratio;
			if (_nearZero(p_e.trigger.value, kTriggerDeadzone))
			{
				ControllerInputThread::PostLeftTriggerReset(p_e.window->_hwnd);
			}
			return;

		case spk::Controller::Trigger::ID::Right:
			_controller._rightTrigger.ratio = ratio;
			if (_nearZero(p_e.trigger.value, kTriggerDeadzone))
			{
				ControllerInputThread::PostRightTriggerReset(p_e.window->_hwnd);
			}
			return;

		default:
			GENERATE_ERROR("Unexpected trigger id.");
		}
	}

	void ControllerModule::_handleDirectionalCrossMotion(const spk::ControllerEvent &p_e)
	{
		_controller._window = p_e.window;
		_controller._directionalCross = p_e.directionalCross.values;

		if (_inDeadzone2D(p_e.directionalCross.values, kJoystickDeadzone))
		{
			ControllerInputThread::PostDirectionalCrossReset(p_e.window->_hwnd);
		}
	}

	void ControllerModule::_handlePress(const spk::ControllerEvent &p_e)
	{
		if (p_e.button == spk::Controller::Button::Unknow)
		{
			return;
		}
		_controller._buttons[static_cast<int>(p_e.button)] = spk::InputState::Down;
	}

	void ControllerModule::_handleRelease(const spk::ControllerEvent &p_e)
	{
		if (p_e.button == spk::Controller::Button::Unknow)
		{
			return;
		}
		_controller._buttons[static_cast<int>(p_e.button)] = spk::InputState::Up;
	}

	void ControllerModule::_handleJoystickReset(const spk::ControllerEvent &p_e)
	{
		switch (p_e.joystick.id)
		{
		case spk::Controller::Joystick::ID::Left:
			_controller._leftJoystick.delta = 0;
			_controller._leftJoystick.position = 0;
			return;
		case spk::Controller::Joystick::ID::Right:
			_controller._rightJoystick.delta = 0;
			_controller._rightJoystick.position = 0;
			return;
		default:
			GENERATE_ERROR("Unexpected joystick id.");
		}
	}

	void ControllerModule::_handleTriggerReset(const spk::ControllerEvent &p_e)
	{
		switch (p_e.trigger.id)
		{
		case spk::Controller::Trigger::ID::Left:
			_controller._leftTrigger.ratio = 0.0f;
			return;
		case spk::Controller::Trigger::ID::Right:
			_controller._rightTrigger.ratio = 0.0f;
			return;
		default:
			GENERATE_ERROR("Unexpected trigger id.");
		}
	}

	void ControllerModule::_handleDirectionalCrossReset(const spk::ControllerEvent &)
	{
		_controller._directionalCross = 0;
	}

	void ControllerModule::_treatEvent(spk::ControllerEvent &&p_event)
	{
		switch (p_event.type)
		{
		case spk::ControllerEvent::Type::JoystickMotion:
			_handleJoystickMotion(p_event);
			break;
		case spk::ControllerEvent::Type::TriggerMotion:
			_handleTriggerMotion(p_event);
			break;
		case spk::ControllerEvent::Type::DirectionalCrossMotion:
			_handleDirectionalCrossMotion(p_event);
			break;
		case spk::ControllerEvent::Type::Press:
			_handlePress(p_event);
			break;
		case spk::ControllerEvent::Type::Release:
			_handleRelease(p_event);
			break;
		case spk::ControllerEvent::Type::JoystickReset:
			_handleJoystickReset(p_event);
			break;
		case spk::ControllerEvent::Type::TriggerReset:
			_handleTriggerReset(p_event);
			break;
		case spk::ControllerEvent::Type::DirectionalCrossReset:
			_handleDirectionalCrossReset(p_event);
			break;
		default:
			GENERATE_ERROR("Unexpected event type.");
		}

		p_event.controller = &_controller;
		if (auto focus = spk::Widget::focusedWidget(Widget::FocusType::ControllerFocus))
		{
			focus->onControllerEvent(p_event);
		}
		else
		{
			p_event.window->widget()->onControllerEvent(p_event);
		}
	}

	spk::ControllerEvent ControllerModule::_convertEventToEventType(spk::Event &&p_event)
	{
		return p_event.controllerEvent;
	}

	ControllerModule::ControllerModule() = default;

	const spk::Controller &ControllerModule::controller() const
	{
		return _controller;
	}
}
