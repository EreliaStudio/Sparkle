#pragma once

#include "math/spk_vector2.hpp"

namespace spk::widget
{
    struct BoxConstraints
    {
        Vector2 min;
        Vector2 max;

        BoxConstraints(const Vector2& p_min, const Vector2& p_max) :
            min(p_min),
            max(p_max)
        {
        }
    };
}
