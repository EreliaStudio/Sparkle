#pragma once

#include "input/spk_input_state.hpp"
#include "math/spk_vector2.hpp"

namespace spk
{
	/**
	 * @class Mouse
	 * @brief Manages mouse input, including button states, position, and wheel movement.
	 *
	 * This class tracks the state of mouse buttons (pressed, released, up, down), the position of the mouse cursor, and the movement of the mouse wheel. It provides methods to update these states based on input events, making it suitable for integrating mouse-based interaction within applications.
	 *
	 * The Mouse class supports a fixed number of buttons and uses an enum to identify these buttons for clarity in handling their state. The position and wheel movement are represented as two-dimensional vectors, allowing for detailed tracking of cursor movement and scroll wheel usage.
	 *
	 * Usage example:
	 * @code
	 * const spk::Mouse& mouse = myApplication.mouse();
	 *
	 * //Getting the mouse position
	 * spk::Vector2Int mousePosition = mouse.position();
	 * 
	 * // Checking button state
	 * if (mouse.getButton(spk::Mouse::Button::Left) == spk::InputState::Pressed) {
	 *     // Respond to left button press
	 * }
	 *
	 * // Reading cursor movement
	 * spk::Vector2Int deltaPosition = mouse.deltaPosition();
	 * if (deltaPosition != spk::Vector2Int(0, 0)) {
	 *     // Handle cursor movement
	 * }
	 *
	 * // Handling wheel scrolling
	 * spk::Vector2Int wheelMovement = mouse.wheel();
	 * if (wheelMovement.y != 0) {
	 *     // Scroll content based on wheel movement
	 * }
	 * @endcode
	 *
	 * @note This class is hold by spk::Application, and accessible from anywhere in the code as const Mouse& via spk::Application::activeApplication()->mouse().
	 * 
	 * @see Application, InputState, Vector2Int
	 */
	class Mouse
	{
	public:
		enum Button
		{
			Left,
			Middle,
			Right,
			Button3,
			Button4
		};

		static const size_t NB_BUTTON = 5;

	private:
		Vector2Int INITIAL_DELTA_POSITION = Vector2Int(-10000, -10000);
		InputState _buttons[NB_BUTTON];

		Vector2Int _position;
		Vector2Int _deltaPosition = INITIAL_DELTA_POSITION;
		Vector2Int _wheel;

	private:

	public:
		Mouse();
		~Mouse();

		void setMousePosition(const Vector2Int& p_newPosition);
		void pressButton(const Button& p_button);
		void releaseButton(const Button& p_button);
		void editWheelPosition(const Vector2Int& p_delta);

		void update();

		const InputState& getButton(const Button& p_button) const;

		const Vector2Int& position() const;
		const Vector2Int& deltaPosition() const;
		const Vector2Int& wheel() const;
	};
}