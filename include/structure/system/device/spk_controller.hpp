#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/system/device/spk_input_state.hpp"

namespace spk
{
	/**
	 * @struct Controller
	 * @brief Represents the state of a game controller, including buttons, joysticks, triggers, and directional cross.
	 *
	 * The `Controller` struct provides a comprehensive representation of a game controller's state,
	 * allowing for tracking input from buttons, joysticks, triggers, and directional input.
	 *
	 * ### Example
	 * @code
	 * spk::Controller controller;
	 * controller.leftJoystick.position = spk::Vector2Int(100, 200);
	 * controller.buttons[static_cast<int>(spk::Controller::Button::A)].setPressed(true);
	 * @endcode
	 */
	struct Controller
	{
		/**
		 * @enum Button
		 * @brief Enumerates the buttons available on a game controller.
		 */
		enum class Button
		{
			A,		   ///< Button A.
			B,		   ///< Button B.
			X,		   ///< Button X.
			Y,		   ///< Button Y.
			LeftArrow,   ///< Left directional arrow.
			DownArrow,   ///< Down directional arrow.
			RightArrow,  ///< Right directional arrow.
			UpArrow,	 ///< Up directional arrow.
			Start,	   ///< Start button.
			Select,	  ///< Select button.
			R1,		  ///< Right shoulder button 1.
			L1,		  ///< Left shoulder button 1.
			R3,		  ///< Right joystick press.
			L3,		  ///< Left joystick press.
			Unknow	   ///< Unknown button.
		};

		/**
		 * @struct Joystick
		 * @brief Represents the state of a joystick, including position and delta movement.
		 */
		struct Joystick
		{
			/**
			 * @enum ID
			 * @brief Identifies the joysticks on a controller.
			 */
			enum ID
			{
				Left,  ///< Left joystick.
				Right  ///< Right joystick.
			};

			Vector2Int delta;	///< Delta movement of the joystick.
			Vector2Int position; ///< Current position of the joystick.
		};

		/**
		 * @struct Trigger
		 * @brief Represents the state of a trigger, including its activation ratio.
		 */
		struct Trigger
		{
			/// Maximum value for trigger input.
			static const inline float MaxValue = static_cast<float>(std::numeric_limits<unsigned short>::max());

			/**
			 * @enum ID
			 * @brief Identifies the triggers on a controller.
			 */
			enum ID
			{
				Left,  ///< Left trigger.
				Right  ///< Right trigger.
			};

			float ratio; ///< Activation ratio of the trigger (0.0 to 1.0).
		};

		Joystick leftJoystick; ///< State of the left joystick.
		Joystick rightJoystick; ///< State of the right joystick.
		Trigger leftTrigger; ///< State of the left trigger.
		Trigger rightTrigger; ///< State of the right trigger.
		Vector2Int directionalCross; ///< State of the directional cross.
		InputState buttons[17]; ///< Array representing the state of all buttons.
	};
}

/**
 * @brief Outputs a string representation of a `Controller::Button` to a stream.
 * @param p_os The output stream.
 * @param p_button The button to output.
 * @return Reference to the output stream.
 */
std::ostream& operator << (std::ostream& p_os, const spk::Controller::Button& p_button);

/**
 * @brief Outputs a string representation of a `Controller::Joystick::ID` to a stream.
 * @param p_os The output stream.
 * @param p_joystickID The joystick ID to output.
 * @return Reference to the output stream.
 */
std::ostream& operator << (std::ostream& p_os, const spk::Controller::Joystick::ID& p_joystickID);

/**
 * @brief Outputs a string representation of a `Controller::Trigger::ID` to a stream.
 * @param p_os The output stream.
 * @param p_triggerID The trigger ID to output.
 * @return Reference to the output stream.
 */
std::ostream& operator << (std::ostream& p_os, const spk::Controller::Trigger::ID& p_triggerID);

/**
 * @brief Outputs a string representation of a `Controller::Button` to a wide stream.
 * @param p_os The wide output stream.
 * @param p_button The button to output.
 * @return Reference to the wide output stream.
 */
std::wostream& operator << (std::wostream& p_os, const spk::Controller::Button& p_button);

/**
 * @brief Outputs a string representation of a `Controller::Joystick::ID` to a wide stream.
 * @param p_os The wide output stream.
 * @param p_joystickID The joystick ID to output.
 * @return Reference to the wide output stream.
 */
std::wostream& operator << (std::wostream& p_os, const spk::Controller::Joystick::ID& p_joystickID);

/**
 * @brief Outputs a string representation of a `Controller::Trigger::ID` to a wide stream.
 * @param p_os The wide output stream.
 * @param p_triggerID The trigger ID to output.
 * @return Reference to the wide output stream.
 */
std::wostream& operator << (std::wostream& p_os, const spk::Controller::Trigger::ID& p_triggerID);
