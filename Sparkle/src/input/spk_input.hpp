#pragma once

#include <functional>
#include "input/spk_input_state.hpp"
#include "input/spk_keyboard.hpp"
#include "input/spk_mouse.hpp"
#include "system/spk_timer.hpp"

namespace spk
{
    /**
     * @class Input
     * @brief Manages generic input triggers and their associated callbacks.
     *
     * The Input class provides a framework for handling various types of input triggers,
     * using timers and functional callbacks to execute actions based on input conditions.
     * It supports configurable delay for trigger activation and allows dynamic adjustment of
     * trigger conditions and associated actions.
     *
     * This class is designed to be extended for specific input devices like keyboards and mice.
     * It utilizes a functional approach to define trigger conditions and callbacks, making it
     * flexible and easy to integrate into various parts of an application.
     *
     * Usage example:
     * @code
     * // Define a trigger condition and callback
     * spk::Input::TriggerCallback onKeyPress = []() { return spk::Keyboard::isKeyPressed(spk::Keyboard::Key::Space); };
     * spk::Input::Callback onAction = []() { std::cout << "Space key pressed!" << std::endl; };
     *
     * // Create an Input object with a 500 ms delay
     * spk::Input spaceKeyInput(onKeyPress, 500, onAction);
     *
     * // In your main update loop, call update to check and execute the input action
     * spaceKeyInput.update();
     * @endcode
     *
     * @note Ensure that the update method is called regularly to properly handle trigger checks.
     */
    class Input
    {
    public:
        using TriggerCallback = std::function<bool()>; ///< Defines a function type that evaluates to a boolean, used to determine if the input condition has been met.
        using Callback = std::function<void()>; ///< Defines a function type that is called when the input condition is met, performing the specified action.

    private:
        TriggerCallback _trigger;  ///< Condition under which the callback is triggered.
        spk::Timer _timer;         ///< Timer to manage trigger delay.
        Callback _callback;        ///< Action to perform when the trigger condition is met.

    public:
        /**
         * Constructs an Input object with specified trigger conditions and actions.
         * @param p_trigger Function to determine when to trigger the callback.
         * @param p_triggerDelay Delay before the trigger can activate, in milliseconds.
         * @param p_callback Function to call when the trigger condition is met.
         */
        Input(const TriggerCallback& p_trigger, const long long& p_triggerDelay, const Callback& p_callback);

        /**
         * Updates the internal state and checks the trigger condition.
         */
        void update();

        /**
         * Sets the delay for the trigger.
         * @param p_triggerDelay Delay in milliseconds before the trigger can activate.
         */
        void setTriggerDelay(const long long& p_triggerDelay);

        /**
         * Sets the function that determines when to trigger the callback.
         * @param p_trigger A function returning a bool to set as the new trigger condition.
         */
        void setTriggerCallback(const TriggerCallback& p_trigger);

        /**
         * Sets the callback function to be called when the trigger condition is met.
         * @param p_callback A function to set as the callback.
         */
        void setCallback(const Callback& p_callback);

        /**
         * Returns a copy of this Input object.
         * @return A new instance of Input with the same properties.
         */
        Input toInput() const;
    };

    /**
     * @class KeyboardInput
     * @brief Specialized Input class for handling keyboard-specific triggers.
     *
     * This class extends the generic Input class to specifically handle keyboard events,
     * such as key presses and releases (Uppon update call). It allows defining a specific key and its expected state
     * (pressed or released) as a trigger for executing a callback. The trigger delay can be
     * set to manage when the action should be executed after the key state matches the expected state.
     *
     * Usage example:
     * @code
     * // Define a callback for when the 'A' key is pressed
     * spk::Input::Callback onKeyPress = []() { std::cout << "Key 'A' was pressed." << std::endl; };
     *
     * // Create a KeyboardInput object for the 'A' key, detecting when it is pressed
     * spk::KeyboardInput aKeyInput(spk::Keyboard::A, spk::InputState::Pressed, 100, onKeyPress);
     *
     * // In your main update loop, call update to check and execute the input action
     * aKeyInput.update();
     *
     * // Change the key and state to listen to the 'B' key released
     * aKeyInput.rebind(spk::Keyboard::Key::B, spk::InputState::Released);
     * @endcode
     *
     * @note Ensure that the update method is called regularly to properly handle trigger checks.
     */
    class KeyboardInput : public Input
    {
    private:
        using Input::setTriggerCallback;  ///< Hide the generic setTriggerCallback method.

    public:
        /**
         * Constructs a KeyboardInput object with specified key, state, delay, and action.
         * @param p_key Keyboard key that triggers the callback.
         * @param p_expectedStatus The state of the key (pressed or released) that should trigger the callback.
         * @param p_delay Delay before the trigger can activate, in milliseconds.
         * @param p_callback The function to execute when the key state matches the expected state.
         */
        KeyboardInput(const spk::Keyboard::Key& p_key, const spk::InputState& p_expectedStatus, const long long& p_delay, const Callback& p_callback);

        /**
         * Rebinds the input to a new key and/or state.
         * @param p_key New keyboard key to monitor.
         * @param p_expectedStatus New expected state of the key (pressed or released).
         */
        void rebind(const spk::Keyboard::Key& p_key, const spk::InputState& p_expectedStatus);
    };

    /**
     * @class KeyboardCharInput
     * @brief Specializes Input for handling character inputs from the keyboard.
     *
     * This class is designed to handle character-specific keyboard inputs, allowing actions
     * to be triggered based on character input rather than specific key presses or states (Uppon update call). 
     * It is useful for capturing user text input in situations like text fields or chat interfaces.
     *
     * Usage example:
     * @code
     * // Define a callback to handle character input
     * spk::Input::Callback onCharInput = []() { std::cout << "Character input received." << std::endl; };
     *
     * // Create a KeyboardCharInput object with the specified callback
     * spk::KeyboardCharInput charInput(onCharInput);
     *
     * // In your main update loop, call update to check and execute the input action
     * charInput.update();
     * @endcode
     *
     * @note Since this class is intended for character input, it does not require setting a trigger condition
     * or delay, which are managed internally based on character input events.
     */
    class KeyboardCharInput : public Input
    {
    private:
        using Input::setTriggerCallback;  ///< Prevents changing the trigger callback directly.
        using Input::setTriggerDelay;     ///< Prevents setting a delay, as it's irrelevant for character input.

    public:
        /**
         * Constructs a KeyboardCharInput object with a specified action to be triggered by character input.
         * @param p_callback The function to execute upon character input detection.
         */
        KeyboardCharInput(const Callback& p_callback);
    };

    /**
     * @class MouseInput
     * @brief Specialized Input class for handling mouse button events.
     *
     * MouseInput extends the generic Input class to specifically manage input triggers related
     * to mouse button actions (Uppon update call). It allows setting up triggers based on the state of specific mouse buttons
     * (pressed or released). This class is ideal for interactive UI elements or game controls that rely on
     * mouse interactions.
     *
     * Usage example:
     * @code
     * // Define a callback for when the left mouse button is pressed
     * spk::Input::Callback onMouseClick = []() { std::cout << "Left mouse button clicked." << std::endl; };
     *
     * // Create a MouseInput object for the left mouse button, detecting when it is pressed
     * spk::MouseInput leftClickInput(spk::Mouse::Left, spk::InputState::Pressed, 100, onMouseClick);
     *
     * // In your main update loop, call update to check and execute the input action
     * leftClickInput.update();
     *
     * // Rebind to the right mouse button if needed
     * leftClickInput.rebind(spk::Mouse::Right, spk::InputState::Pressed);
     * @endcode
     *
     * @note Ensure that the update method is called regularly to properly handle trigger checks.
     */
    class MouseInput : public Input
    {
    private:
        using Input::setTriggerCallback;  ///< Hide the generic setTriggerCallback method.

    public:
        /**
         * Constructs a MouseInput object with specified mouse button, state, delay, and action.
         * @param p_button Mouse button that triggers the callback.
         * @param p_expectedStatus The state of the mouse button (pressed or released) that should trigger the callback.
         * @param p_delay Delay before the trigger can activate, in milliseconds.
         * @param p_callback The function to execute when the mouse button state matches the expected state.
         */
        MouseInput(const spk::Mouse::Button& p_button, const spk::InputState& p_expectedStatus, const long long& p_delay, const Callback& p_callback);

        /**
         * Rebinds the mouse input to a new button and/or state.
         * @param p_button New mouse button to monitor.
         * @param p_expectedStatus New expected state of the mouse button (pressed or released).
         */
        void rebind(const spk::Mouse::Button& p_button, const spk::InputState& p_expectedStatus);
    };

    /**
     * @class MouseMotionInput
     * @brief Specialized Input class for handling mouse motion events.
     *
     * MouseMotionInput extends the Input class to handle triggers based on mouse movement,
     * rather than button presses or keyboard inputs. It is particularly useful for applications
     * that require interaction based on mouse movement, such as navigating through a 3D
     * environment or drawing applications. The callback is triggered on each update if
     * mouse movement is detected (Uppon update call).
     *
     * Usage example:
     * @code
     * // Define a callback to handle mouse motion
     * spk::Input::Callback onMotion = []() { std::cout << "Mouse is moving." << std::endl; };
     *
     * // Create a MouseMotionInput object with the specified callback
     * spk::MouseMotionInput motionInput(onMotion);
     *
     * // In your main update loop, call update to check and execute the input action
     * motionInput.update();
     * @endcode
     *
     * @note Since this class focuses on motion, no trigger conditions or delays are settable,
     * and the callback is directly tied to the mouse movement detection logic.
     */
    class MouseMotionInput : public Input
    {
    private:
        using Input::setTriggerCallback;  ///< Prevents changing the trigger callback directly.
        using Input::setTriggerDelay;     ///< Prevents setting a delay, as it's irrelevant for motion input.

    public:
        /**
         * Constructs a MouseMotionInput object with a specified action to be triggered by mouse movement.
         * @param p_callback The function to execute upon detecting mouse movement.
         */
        MouseMotionInput(const Callback& p_callback);
    };
}