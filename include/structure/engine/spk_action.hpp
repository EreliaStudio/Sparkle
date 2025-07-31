#pragma once

#include "structure/system/event/spk_event.hpp"

#include "structure/spk_safe_pointer.hpp"

#include "structure/system/spk_input_state.hpp"

#include "structure/system/time/spk_timer.hpp"

#include <functional>

#include "structure/system/device/spk_controller.hpp"
#include "structure/system/device/spk_keyboard.hpp"
#include "structure/system/device/spk_mouse.hpp"

#include "structure/math/spk_vector2.hpp"

namespace spk
{
	class Action
	{
	public:
		virtual ~Action() = default;
		virtual bool isInitialized() const = 0;
		virtual void initialize(spk::UpdateEvent &p_event) = 0;
		virtual void update() = 0;
	};

	template <typename Device, typename Event>
	class InputAction : public Action
	{
	public:
		using Job = std::function<void(const spk::SafePointer<const spk::Keyboard> &)>;

	private:
		spk::SafePointer<const Device> _device;

		Event _event;
		spk::InputState _targetState;

		long long _repeatInterval;
		spk::Timer _timer;
		Job _onTriggerCallback;

		bool _lastState = false;

	public:
		InputAction(Event p_event, spk::InputState p_state, long long p_repeatInterval, const Job &p_callback) :
			_event(p_event),
			_targetState(p_state),
			_repeatInterval(p_repeatInterval),
			_timer(p_repeatInterval > 0 ? p_repeatInterval : 0),
			_onTriggerCallback(p_callback)
		{
		}

		bool isInitialized() const override
		{
			return (_device != nullptr);
		}

		void initialize(spk::UpdateEvent &p_event) override
		{
			if constexpr (std::is_same_v<Device, spk::Mouse>)
			{
				_device = p_event.mouse;
			}
			else if constexpr (std::is_same_v<Device, spk::Keyboard>)
			{
				_device = p_event.keyboard;
			}
			else if constexpr (std::is_same_v<Device, spk::Controller>)
			{
				_device = p_event.controller;
			}
			else
			{
				GENERATE_ERROR("Invalid device type");
			}
		}

		spk::SafePointer<const Device> device() const
		{
			return (_device);
		}

		void setDevice(spk::SafePointer<const Device> p_device)
		{
			_device = p_device;
		}

		void update() override
		{
			if (_device == nullptr)
			{
				GENERATE_ERROR("Can't update an InputAction without device");
			}

			spk::InputState currentState = (*_device)[_event];

			bool needTrigger = (currentState == _targetState);
			bool justPressed = needTrigger && _lastState == false;
			_lastState = needTrigger;

			if (justPressed == true)
			{
				_onTriggerCallback(_device);
				if (_repeatInterval > 0)
				{
					_timer.start();
				}
			}
			else if (justPressed == false && needTrigger == true)
			{
				if (_timer.state() != spk::Timer::State::Running)
				{
					_onTriggerCallback(_device);
					if (_repeatInterval > 0)
					{
						_timer.start();
					}
				}
			}
		}
	};

	using KeyboardAction = InputAction<spk::Keyboard, spk::Keyboard::Key>;
	using MouseButtonAction = InputAction<spk::Mouse, spk::Mouse::Button>;
	using ControllerButtonAction = InputAction<spk::Controller, spk::Controller::Button>;

	class MouseMotionAction : public Action
	{
	public:
		enum class Mode
		{
			Absolute,
			Delta
		};

	private:
		std::function<void(const spk::Vector2Int &)> _onTriggerCallback;
		Mode _mode;
		spk::SafePointer<const spk::Mouse> _mouse;

	public:
		MouseMotionAction(Mode p_mode, const std::function<void(const spk::Vector2Int &)> &p_callback);

		bool isInitialized() const override;

		void initialize(spk::UpdateEvent &p_event) override;

		spk::SafePointer<const spk::Mouse> mouse() const;

		void setMouse(spk::SafePointer<const spk::Mouse> p_mouse);

		void update() override;
	};

	// Not working, it seem to have a problem with the spk::Controller
	// class ControllerJoystickAction : public Action
	// {
	// public:
	// 	enum class Mode
	// 	{
	// 		Absolute,
	// 		Delta
	// 	};

	// private:
	// 	spk::Controller::Joystick::ID _joyID;
	// 	Mode _mode;
	// 	std::function<void(const spk::Vector2Int&)> _onTriggerCallback;
	// 	long long _deadZone;
	// 	spk::SafePointer<const spk::Controller> _controller;

	// public:
	// 	ControllerJoystickAction(spk::Controller::Joystick::ID p_joyID, Mode p_mode, const long long p_deadZone, const std::function<void(const
	// spk::Vector2Int&)>& p_callback) : 		_joyID(p_joyID), 		_mode(p_mode), 		_deadZone(p_deadZone), 		_onTriggerCallback(p_callback)
	// 	{

	// 	}

	// 	bool isInitialized() const override
	// 	{
	// 		return (_controller != nullptr);
	// 	}

	// 	void initialize(spk::UpdateEvent& p_event) override
	// 	{
	// 		_controller = p_event.controller;
	// 	}

	// 	spk::SafePointer<const spk::Controller> controller() const
	// 	{
	// 		return (_controller);
	// 	}

	// 	void setController(spk::SafePointer<const spk::Controller> p_controller)
	// 	{
	// 		_controller = p_controller;
	// 	}

	// 	void update() override
	// 	{
	// 		if (_controller == nullptr)
	// 		{
	// 			GENERATE_ERROR("Can't update ControllerJoystickAction without a bound Controller");
	// 		}

	// 		const auto& joy = (_joyID == spk::Controller::Joystick::Left ? _controller->leftJoystick : _controller->rightJoystick);

	// 		if (_mode == Mode::Absolute)
	// 		{
	// 			if (std::abs(joy.position.x) > _deadZone ||
	// 				std::abs(joy.position.y) > _deadZone)
	// 			{
	// 				_onTriggerCallback(joy.position);
	// 			}
	// 		}
	// 		else
	// 		{
	// 			_onTriggerCallback(joy.delta);
	// 		}
	// 	}
	// };
}