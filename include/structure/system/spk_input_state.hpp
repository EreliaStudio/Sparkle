#pragma once

#include <iostream>

#include "spk_debug_macro.hpp"

namespace spk
{
	enum class InputState
	{
		Down,
		Up
	};

	std::ostream &operator<<(std::ostream &p_os, const spk::InputState &p_state);
	std::wostream &operator<<(std::wostream &p_os, const spk::InputState &p_state);
}
