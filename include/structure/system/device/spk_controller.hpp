#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/system/spk_input_state.hpp"

namespace spk
{
	struct Controller
	{
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

		Joystick leftJoystick;
		Joystick rightJoystick;
		Trigger leftTrigger;
		Trigger rightTrigger;
		Vector2Int directionalCross = 0;
		InputState buttons[17];

		InputState operator[](Button p_button) const
		{
			return (buttons[static_cast<int>(p_button)]);
		}
	};
}

std::ostream &operator<<(std::ostream &p_os, const spk::Controller::Button &p_button);
std::ostream &operator<<(std::ostream &p_os, const spk::Controller::Joystick::ID &p_joystickID);
std::ostream &operator<<(std::ostream &p_os, const spk::Controller::Trigger::ID &p_triggerID);

std::wostream &operator<<(std::wostream &p_os, const spk::Controller::Button &p_button);
std::wostream &operator<<(std::wostream &p_os, const spk::Controller::Joystick::ID &p_joystickID);
std::wostream &operator<<(std::wostream &p_os, const spk::Controller::Trigger::ID &p_triggerID);