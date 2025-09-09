#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/spk_safe_pointer.hpp"
#include "structure/system/spk_input_state.hpp"

#include <limits>

namespace spk
{
	class Window;
	class ControllerModule;

	class Controller
	{
		friend class ControllerModule;

	public:
		enum class Button
		{
			A,
			B,
			X,
			Y,
			LeftArrow,
			DownArrow,
			RightArrow,
			UpArrow,
			Start,
			Select,
			R1,
			L1,
			R3,
			L3,
			Unknow
		};

		struct Joystick
		{
			enum ID
			{
				Left,
				Right
			};
			Vector2Int delta = 0;
			Vector2Int position = 0;
		};

		struct Trigger
		{
			static const inline float MaxValue = static_cast<float>(std::numeric_limits<unsigned short>::max());
			enum ID
			{
				Left,
				Right
			};
			float ratio = 0;
		};

	private:
		Joystick _leftJoystick;
		Joystick _rightJoystick;
		Trigger _leftTrigger;
		Trigger _rightTrigger;
		Vector2Int _directionalCross = 0;
		std::array<InputState, 17> _buttons;
		spk::SafePointer<Window> _window;

	public:
		Controller();
		virtual ~Controller() = default;

		InputState operator[](Button p_button) const;

		const Joystick &leftJoystick() const;
		const Joystick &rightJoystick() const;
		const Trigger &leftTrigger() const;
		const Trigger &rightTrigger() const;
		const Vector2Int &directionalCross() const;
		const std::array<InputState, 17>& buttons() const;
		spk::SafePointer<Window> window() const;
	};
}

std::ostream &operator<<(std::ostream &p_os, const spk::Controller::Button &p_button);
std::ostream &operator<<(std::ostream &p_os, const spk::Controller::Joystick::ID &p_joystickID);
std::ostream &operator<<(std::ostream &p_os, const spk::Controller::Trigger::ID &p_triggerID);

std::wostream &operator<<(std::wostream &p_os, const spk::Controller::Button &p_button);
std::wostream &operator<<(std::wostream &p_os, const spk::Controller::Joystick::ID &p_joystickID);
std::wostream &operator<<(std::wostream &p_os, const spk::Controller::Trigger::ID &p_triggerID);
