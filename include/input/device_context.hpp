#pragma once

#include "input/keyboard.hpp"
#include "input/mouse.hpp"

namespace spk
{
	struct Keyboard;
	struct Mouse;

	struct DeviceContext
	{
		const Keyboard &keyboard;
		const Mouse &mouse;
	};
}
