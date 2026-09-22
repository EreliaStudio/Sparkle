#pragma once

#include <chrono>

#include "diagnostics/profiler.hpp"

namespace spk
{
	struct UpdateContext
	{
		std::chrono::steady_clock::duration time;
		std::chrono::steady_clock::duration deltaTime;
		Profiler &profiler = Profiler::defaultProfiler();
	};
}
