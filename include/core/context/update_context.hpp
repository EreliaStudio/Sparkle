#pragma once

#include <chrono>

namespace spk
{
	struct Keyboard;
	struct Mouse;

	struct UpdateContext
	{
		std::chrono::steady_clock::duration time;
		std::chrono::steady_clock::duration deltaTime;
		const spk::Keyboard &keyboard;
		const spk::Mouse &mouse;
	};
}
