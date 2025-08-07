#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/system/spk_input_state.hpp"
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class Window;

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

		Mouse();
		InputState operator[](Button p_button) const;

		void place(const spk::SafePointer<Window>& p_window, const spk::Vector2Int& p_newPosition) const;
	};
}

std::ostream &operator<<(std::ostream &p_os, const spk::Mouse::Button &p_button);
std::wostream &operator<<(std::wostream &p_os, const spk::Mouse::Button &p_button);