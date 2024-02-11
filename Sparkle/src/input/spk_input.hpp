#pragma once

#include "input/spk_mouse.hpp"
#include "input/spk_keyboard.hpp"

namespace spk
{
    /**
     * @brief Input offers a unified interface for input events of different sources.
     *
     * This allows treating inputs from Mouse, Keyboard or Controller with the same methods.
     *
     * @see Keyboard, Mouse, InputState
     */
    class Input
    {
    public:
        /**
         * @brief The Type of the Input. Will be set by the constructor.
         */
        enum Type
        {
            None,
            Mouse,
            Keyboard,
            Controller,
            Other
        };

        /**
         * @brief Default constructor. The resulting Input cannot be used for anything practical.
         */
        Input() : _type(Type::None), _state(InputState::Unknown) {}

        /**
         * @brief Constructs an Input from a Mouse::Button.
         *
         * @see Mouse
         */
        Input(Mouse::Button p_mouseButton, InputState p_state)
            : _type(Type::Mouse), _mouseButton(p_mouseButton), _state(p_state) {}

        /**
         * @brief Constructs an Input from a Keyboard::Key.
         *
         * @see Keyboard
         */
        Input(Keyboard::Key p_key, InputState p_state)
            : _type(Type::Keyboard), _key(p_key), _state(p_state) {}

        using Code = size_t;

        static constexpr Code kCodeUnknown = 0; //< A code that is not used by any Input.

        static constexpr Code max_code = Keyboard::Key::Maximum + Mouse::Button::Maximum; //< The largest possible input code.

        /**
         * @brief Getter for Code.
         *
         * A code is unique to all input types, which means that two Inputs with the same code
         * have the same type and the same value inside that type.
         */
        Code code() const
        {
            switch (_type)
            {
            case Type::Keyboard:
                return 1 + _key;
            case Type::Mouse:
                return 1 + _mouseButton + Keyboard::Key::Maximum;
            case Type::Controller:
                // return 1 + _key + Keyboard::Key::Maximum + Mouse::Button::Maximum;
            case Type::Other:
            case Type::None:
            default:
                return kCodeUnknown;
            }
        }

        /**
         * @brief Getter for the InputState.
         */
        InputState state() const { return _state; }

        bool operator==(const Input &other) const
        {
            return (code() == other.code()) && (state() == other.state());
        }

    private:
        Type _type;
        Mouse::Button _mouseButton;
        Keyboard::Key _key;
        InputState _state;
    };
}