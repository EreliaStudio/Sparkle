#pragma once

#include "input/spk_input_state.hpp"
#include <atomic>
#include <map>
#include <vector>

namespace spk
{
    /**
     * @class Keyboard
     * @brief Manages keyboard input states and character input for an application.
     *
     * This class provides functionality to track the state of individual keys on the keyboard,
     * including whether they are currently pressed, released, up, or down. It also handles
     * character input, allowing for the capture of typed characters in a thread-safe manner
     * using atomic operations.
     *
     * The Keyboard class defines an enum for key codes that represent various keyboard keys,
     * including alphanumeric keys, function keys, control keys, and more.
     * This enables the application to respond to specific key events and manage
     * keyboard-based interaction efficiently.
     *
     * Usage example:
     * @code
     * const spk::Keyboard& keyboard = myApplication.keyboard();
     *
     * // In an input handling loop
     * if (keyboard.getKey(spk::Keyboard::Key::A) == spk::InputState::Pressed) {
     *     // Respond to 'A' key press
     * }
     *
     * wchar_t typedChar = keyboard.getChar();
     * if (typedChar != L'\0') {
     *     // Handle character input
     * }
     * @endcode
     *
     * @note This class is hold by spk::Application, and accessible from anywhere in the code as const Keyboard& via spk::Application::activeApplication()->keyboard().
     *
     * @see Application, InputStatus
     */
    class Keyboard
    {
    public:
        /**
         * @enum Key
         * @brief Represents the key codes for various keyboard keys.
         *
         * This enumeration provides a comprehensive list of key codes representing alphanumeric keys, function keys, control keys, and more. It facilitates the handling of keyboard input by providing a clear mapping of physical keys to their logical representations within the application.
         */
        enum Key
        {
            Backspace = 8,
            Tab = 9,
            Clear = 12,
            Return = 13,
            Shift = 16,
            Control = 17,
            Alt = 18,
            Pause = 19,
            Capslock = 20,
            Escape = 27,
            Convert = 28,
            Non_convert = 29,
            Accept = 30,
            Mode_change = 31,
            Space = 32,
            Prior = 33,
            Next = 34,
            End = 35,
            Home = 36,
            LeftArrow = 37,
            UpArrow = 38,
            RightArrow = 39,
            DownArrow = 40,
            Select = 41,
            Print = 42,
            Execute = 43,
            Snapshot = 44,
            Insert = 45,
            Delete = 46,
            Help = 47,
            Key0 = 48,
            Key1 = 49,
            Key2 = 50,
            Key3 = 51,
            Key4 = 52,
            Key5 = 53,
            Key6 = 54,
            Key7 = 55,
            Key8 = 56,
            Key9 = 57,
            A = 65,
            B = 66,
            C = 67,
            D = 68,
            E = 69,
            F = 70,
            G = 71,
            H = 72,
            I = 73,
            J = 74,
            K = 75,
            L = 76,
            M = 77,
            N = 78,
            O = 79,
            P = 80,
            Q = 81,
            R = 82,
            S = 83,
            T = 84,
            U = 85,
            V = 86,
            W = 87,
            X = 88,
            Y = 89,
            Z = 90,
            LeftWindows = 91,
            RightWindows = 92,
            App = 93,
            Sleep = 95,
            Numpad0 = 96,
            Numpad1 = 97,
            Numpad2 = 98,
            Numpad3 = 99,
            Numpad4 = 100,
            Numpad5 = 101,
            Numpad6 = 102,
            Numpad7 = 103,
            Numpad8 = 104,
            Numpad9 = 105,
            NumpadMultiply = 106,
            NumpadPlus = 107,
            NumpadSeparator = 108,
            NumpadMinus = 109,
            NumpadDecimal = 110,
            NumpadDivide = 111,
            F1 = 112,
            F2 = 113,
            F3 = 114,
            F4 = 115,
            F5 = 116,
            F6 = 117,
            F7 = 118,
            F8 = 119,
            F9 = 120,
            F10 = 121,
            F11 = 122,
            F12 = 123,
            F13 = 124,
            F14 = 125,
            F15 = 126,
            F16 = 127,
            F17 = 128,
            F18 = 129,
            F19 = 130,
            F20 = 131,
            F21 = 132,
            F22 = 133,
            F23 = 134,
            F24 = 135,
            NumLock = 144,
            Scroll = 145,
            LeftShift = 160,
            RightShift = 161,
            LeftControl = 162,
            RightControl = 163,
            LeftMenu = 164,
            RightMenu = 165,
            SemiColon = 186,
            Plus = 187,
            Comma = 188,
            Minus = 189,
            Period = 190,
            QuestionMark = 191,
            Tilde = 192,
            LeftBracket = 219,
            VerticalLine = 220,
            RightBracket = 221,
            Quote = 222,
            Unknow = 223,
            AngleBracket = 226,
            Process = 229,
            Maximum = 255
        };

    private:
        static inline const char kUp = static_cast<char>(InputState::Up);
        static inline const char kDown = static_cast<char>(InputState::Down);
        static inline const char kPressed = static_cast<char>(InputState::Pressed);
        static inline const char kReleased = static_cast<char>(InputState::Released);

        std::atomic_uint8_t _keys[Keyboard::Key::Maximum];
        std::atomic_wchar_t _char;
        std::vector<uint32_t> _keysToUpdate;

    public:
        /**
         * @brief Constructs a Keyboard object.
         *
         * Initializes a Keyboard instance, setting up internal structures for tracking the state of keyboard keys and capturing character input.
         */
        Keyboard();

        /**
         * @brief Destructs the Keyboard object.
         *
         * Cleans up resources associated with the Keyboard instance. This includes resetting the state of any tracked keys and character input.
         */
        ~Keyboard();

        /**
         * @brief Captures a character input.
         *
         * Stores the most recent character typed on the keyboard in a thread-safe manner. This method is intended to be called by the input handling system whenever a character input event is received.
         *
         * @param p_char The character that was typed.
         */
        void setChar(const wchar_t& p_char);

        /**
         * @brief Marks a keyboard key as pressed.
         *
         * Registers a key press event for the specified key in a thread-safe manner. This method should be called by the input handling system when a key press event is detected.
         *
         * @param p_key The key code of the pressed key.
         */
        void pressKey(uint32_t p_key);

        /**
         * @brief Marks a keyboard key as released.
         *
         * Registers a key release event for the specified key in a thread-safe manner. This method should be called by the input handling system when a key release event is detected.
         *
         * @param p_key The key code of the released key.
         */
        void releaseKey(uint32_t p_key);

        /**
         * @brief Updates the state of all keys.
         *
         * Processes any pending key state updates. This method should be called periodically, typically at the beginning of each frame or input polling cycle, to ensure the keyboard state is accurately reflected.
         */
        void update();

        /**
         * @brief Retrieves the most recent character input.
         *
         * Returns the last character that was typed on the keyboard. If no character has been typed since the last call to `getChar`, it returns L'\0'.
         *
         * @return The most recently typed character, or L'\0' if no new character has been typed.
         */
        wchar_t getChar() const;

        /**
         * @brief Retrieves the state of a specified key.
         *
         * Returns the current state of the specified key, such as pressed, released, up, or down.
         *
         * @param p_key The key for which to retrieve the state.
         * @return The current state of the key as an InputState value.
         */
        InputState getKey(Key p_key) const;
    };
}