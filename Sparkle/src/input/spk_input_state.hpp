#pragma once

#include <cstdint>

namespace spk
{
	enum class InputState : uint8_t
	{
		Unknown,
		Up,
		Pressed,
		Down,
		Released
	};
}