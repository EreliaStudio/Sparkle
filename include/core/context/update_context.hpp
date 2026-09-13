#pragma once

#include <chrono>

#include "diagnostics/profiler.hpp"

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
		Profiler &profiler = Profiler::defaultProfiler();
	};
}
