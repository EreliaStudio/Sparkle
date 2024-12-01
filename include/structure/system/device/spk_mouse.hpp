#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/system/spk_input_state.hpp"

namespace spk
{
	struct Mouse
	{
		enum class Button
		{
			Left,
			Middle,
			Right,
			Unknow
		};

		Vector2Int position;
		Vector2Int deltaPosition;
		InputState buttons[3];
		float wheel;
	};
}

std::ostream& operator << (std::ostream & p_os, const spk::Mouse::Button & p_button);
std::wostream& operator << (std::wostream & p_os, const spk::Mouse::Button & p_button);