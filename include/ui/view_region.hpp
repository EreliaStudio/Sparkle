#pragma once

#include "math/rect2d.hpp"

namespace spk
{
	struct ViewRegion
	{
		spk::Rect2D viewport;
		spk::Rect2D scissor;
	};
}