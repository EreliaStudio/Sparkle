#pragma once

#include "input/spk_keyboard.hpp"

#include <unordered_map>

namespace spk::input
{
    template <class TInputType>
    class KeyboardTranslator
    {
    public:
        KeyboardTranslator(Keyboard* p_keyboard) :
            _keyboard(p_keyboard)
        {
        }

        virtual ~KeyboardTranslator() = default;

        void setMappings(const std::unordered_map<TInputType, Keyboard::Key> p_mappings)
        {
            _mappings = p_mappings;
        }

        void setMapping(TInputType p_input, Keyboard::Key p_key)
        {
            _mappings[p_input] = p_key;
        }

        void clear()
        {
            _mappings.clear();
        }

        void erase(TInputType p_input)
        {
            _mappings.erase(p_input);
        }

        InputState getInput(TInputType input)
        {
            {
                auto it = _mappings.find(input);
                if (it == _mappings.end())
                {
                    return InputState::Unknown;
                }
                return _keyboard->getKey(it->second);
            }
        }

    private:
        Keyboard* _keyboard;
        std::unordered_map<TInputType, Keyboard::Key> _mappings;
    };
}