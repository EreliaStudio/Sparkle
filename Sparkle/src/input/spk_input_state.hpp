#pragma once

#include <cstdint>

namespace spk
{
	enum class InputState : uint8_t
	{
		Up,
		Pressed,
		Down,
		Released
	};
}