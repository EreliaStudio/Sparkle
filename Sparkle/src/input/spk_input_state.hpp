#pragma once

#include <cstdint>
#include <iostream>

namespace spk
{
    enum class InputState : uint8_t
    {
        Unknown,
        Up,
        Pressed,
        Down,
        Released,
        Count
    };

    std::ostream& operator << (std::ostream& p_os, const InputState& p_state);
}