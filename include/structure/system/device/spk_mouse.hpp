#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/system/device/spk_input_state.hpp"

namespace spk
{
	/**
	 * @struct Mouse
	 * @brief Represents the state of a mouse, including position, button states, and wheel movement.
	 *
	 * The `Mouse` struct tracks the current position, movement, and state of buttons for a mouse device.
	 *
	 * ### Example
	 * @code
	 * spk::Mouse mouse;
	 * mouse.position = spk::Vector2Int(100, 200);
	 * mouse.deltaPosition = spk::Vector2Int(5, -3);
	 * mouse.buttons[static_cast<int>(spk::Mouse::Button::Left)].setPressed(true);
	 * mouse.wheel = 1.0f; // Scroll wheel moved up.
	 * @endcode
	 */
	struct Mouse
	{
		/**
		 * @enum Button
		 * @brief Enumerates the mouse buttons.
		 */
		enum class Button
		{
			Left,   ///< Left mouse button.
			Middle, ///< Middle mouse button (scroll wheel button).
			Right,  ///< Right mouse button.
			Unknow  ///< Unknown mouse button.
		};

		Vector2Int position; ///< Current position of the mouse cursor.
		Vector2Int deltaPosition; ///< Change in mouse position since the last update.
		InputState buttons[3]; ///< Array representing the state of the mouse buttons (Left, Middle, Right).
		float wheel; ///< Wheel movement value (positive for scrolling up, negative for scrolling down).
	};

	/**
	 * @brief Outputs a string representation of a `Mouse::Button` to a stream.
	 * @param p_os The output stream.
	 * @param p_button The mouse button to output.
	 * @return Reference to the output stream.
	 */
	std::ostream& operator << (std::ostream& p_os, const spk::Mouse::Button& p_button);

	/**
	 * @brief Outputs a string representation of a `Mouse::Button` to a wide stream.
	 * @param p_os The wide output stream.
	 * @param p_button The mouse button to output.
	 * @return Reference to the wide output stream.
	 */
	std::wostream& operator << (std::wostream& p_os, const spk::Mouse::Button& p_button);
}
