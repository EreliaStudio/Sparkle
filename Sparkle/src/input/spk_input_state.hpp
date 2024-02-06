#pragma once

#include <cstdint>

namespace spk
{
	/**
	 * @brief Represents the state of an input device (e.g., keyboard key, mouse button).
	 *
	 * This enum classifies the various states that an input can be in during application runtime. It is designed to help manage input events more intuitively, making it easier to implement responsive and interactive applications.
	 *
	 * - `Up`: The input is not being pressed.
	 * - `Pressed`: The input has just been pressed down.
	 * - `Down`: The input is currently being pressed down.
	 * - `Released`: The input was pressed but has just been released.
	 *
	 * Usage example: CF Keyboard or Mouse for more information
	 * 
	 * @see Mouse, Keyboard
	 */
	enum class InputState : uint8_t
	{
		Up,
		Pressed,
		Down,
		Released
	};
}