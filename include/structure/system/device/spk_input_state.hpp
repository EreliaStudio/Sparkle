#pragma once

namespace spk
{
	/**
	 * @enum InputState
	 * @brief Represents the state of an input, such as a key or button.
	 *
	 * The `InputState` enum class is used to indicate whether an input is currently pressed (`Down`) or released (`Up`).
	 */
	enum class InputState : bool
	{
		Down = true, ///< Input is pressed.
		Up = false   ///< Input is released.
	};
}
