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
		/**
		 * @brief Constructs a Mouse object.
		 * 
		 * Initializes a Mouse instance, setting up initial states for mouse buttons, position, delta position, and wheel movement.
		 */
		Mouse();

		/**
		 * @brief Destructs the Mouse object.
		 * 
		 * Cleans up any resources or states associated with the Mouse instance.
		 */
		~Mouse();

		/**
		 * @brief Sets the current mouse cursor position.
		 * 
		 * This method updates the internal state of the mouse cursor position. Typically called by the application's input handling system when a mouse move event is detected.
		 * 
		 * @param p_newPosition The new position of the mouse cursor as a Vector2Int.
		 */
		void setMousePosition(const Vector2Int& p_newPosition);

		/**
		 * @brief Registers a mouse button press event.
		 * 
		 * Marks the specified mouse button as pressed in the internal button state array. This method is intended to be called by the input handling system when a mouse button press event is detected.
		 * 
		 * @param p_button The button that was pressed.
		 */
		void pressButton(const Button& p_button);

		/**
		 * @brief Registers a mouse button release event.
		 * 
		 * Marks the specified mouse button as released in the internal button state array. This method is intended to be called by the input handling system when a mouse button release event is detected.
		 * 
		 * @param p_button The button that was released.
		 */
		void releaseButton(const Button& p_button);

		/**
		 * @brief Updates the position of the mouse wheel.
		 * 
		 * Modifies the internal state to reflect the movement of the mouse wheel. Typically called by the application's input handling system when a mouse wheel scroll event is detected.
		 * 
		 * @param p_delta The change in position of the mouse wheel since the last update, represented as a Vector2Int. The y-component usually represents the scroll amount.
		 */
		void editWheelPosition(const Vector2Int& p_delta);

		/**
		 * @brief Updates the internal states of the mouse.
		 * 
		 * Processes any pending updates to the mouse's button states, position, and wheel movement. This method should be called periodically, typically at the beginning of each frame or input polling cycle, to ensure the mouse state is accurately reflected.
		 */
		void update();

		/**
		 * @brief Retrieves the state of a specified mouse button.
		 * 
		 * @param p_button The button for which to retrieve the state.
		 * @return The current state of the button as an InputState value.
		 */
		const InputState& getButton(const Button& p_button) const;

		/**
		 * @brief Gets the current position of the mouse cursor.
		 * 
		 * @return The position of the mouse cursor as a Vector2Int.
		 */
		const Vector2Int& position() const;

		/**
		 * @brief Gets the change in position of the mouse cursor.
		 * 
		 * @return The delta position of the mouse cursor since the last update as a Vector2Int.
		 */
		const Vector2Int& deltaPosition() const;

		/**
		 * @brief Gets the current wheel movement.
		 * 
		 * @return The movement of the mouse wheel since the last update as a Vector2Int. The y-component usually represents the vertical scroll amount.
		 */
		const Vector2Int& wheel() const;
	};
}