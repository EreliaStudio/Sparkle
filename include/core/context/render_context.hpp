#pragma once

#include "core/window.hpp"
#include "diagnostics/profiler.hpp"

namespace spk
{
	struct RenderContext
	{
		Window::Surface *targetSurface;
		Profiler &profiler = Profiler::defaultProfiler();
	};
}
