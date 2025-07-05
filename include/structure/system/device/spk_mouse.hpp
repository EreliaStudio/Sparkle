#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/system/spk_input_state.hpp"

namespace spk
{
	struct Mouse
	{
		enum class Button
		{
			Left = 0,
			Middle = 1,
			Right = 2,
			Unknow
		};

		Vector2Int position;
		Vector2Int deltaPosition;
		InputState buttons[3];
		float wheel;

		Mouse()
		{
			for (size_t i = 0; i < 3; i++)
			{
				buttons[i] = spk::InputState::Up;
			}
			position = {0, 0};
			deltaPosition = {0, 0};
			wheel = 0;
		}

		InputState operator[](Button p_button) const
		{
			return (buttons[static_cast<int>(p_button)]);
		}
	};
}

std::ostream &operator<<(std::ostream &p_os, const spk::Mouse::Button &p_button);
std::wostream &operator<<(std::wostream &p_os, const spk::Mouse::Button &p_button);