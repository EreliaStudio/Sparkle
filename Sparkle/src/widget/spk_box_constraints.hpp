#pragma once

#include "math/spk_vector2.hpp"

namespace spk::widget
{
    /**
     * @brief BoxConstraints defines the minimum and maximum size that a box should take.
     */
    struct BoxConstraints
    {
        Vector2 min; //!< minimum size.
        Vector2 max; //!< maximum size.

        /**
         * @brief Constructor.
         *
         * @param p_min The minimum size.
         * @param p_max The maximum size.
         */
        BoxConstraints(const Vector2& p_min, const Vector2& p_max) :
            min(p_min),
            max(p_max)
        {
        }
    };
}
