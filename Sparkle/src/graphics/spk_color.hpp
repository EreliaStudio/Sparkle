#pragma once

#include <iostream>

namespace spk
{
    /**
     * @brief Represents a color in RGBA format.
     *
     * This struct encapsulates a color using the RGBA color model, where R, G, B, and A represent the
     * red, green, blue, and alpha (transparency) components of the color, respectively.
     * It provides constructors for initializing a color from integer or floating-point values and
     * supports basic color arithmetic operations such as addition and subtraction.
     *
     * The Color struct can be used to define colors for graphics rendering, user interface elements,
     * or any other components requiring color specification. The alpha component allows for transparency
     * effects, with a default value of 1.0f (fully opaque) when not specified.
     *
     * Usage example:
     * @code
     * spk::Color white(255, 255, 255); // Opaque white
     * spk::Color semiTransparentRed(1.0f, 0.0f, 0.0f, 0.5f); // Semi-transparent red
     *
     * spk::Color mixedColor = white + semiTransparentRed; // Mix white and red
     * @endcode
     *
     * @note Color components are clamped to the range [0, 255] for integers and [0.0f, 1.0f] for floating-point values.
     */
    struct Color
    {
        float r;
        float g;
        float b;
        float a;

        Color();
        Color(int p_value);
        Color(int p_r, int p_g, int p_b, int p_a = 255);
        Color(float p_r, float p_g, float p_b, float p_a = 1.0f);

        Color operator + (const Color& p_color) const;
        Color operator - (const Color& p_color) const;
    };
}
