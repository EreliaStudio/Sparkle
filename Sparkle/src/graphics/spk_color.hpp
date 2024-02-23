#pragma once

#include <iostream>
#include <vector>

namespace spk
{
    /**
     * @class Color
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
        float r; //!< Red component of the color.
        float g; //!< Green component of the color.
        float b; //!< Blue component of the color.
        float a; //!< Alpha (transparency) component of the color.

        /**
         * @brief Default constructor initializes color to transparent black.
         */
        Color();

        /**
         * @brief Constructor to initialize color with hexadecimal value.
         *
         * @param p_value The hex value of the color.
         */
        Color(int p_value);

        /**
         * @brief Constructor to initialize color from integer values.
         *
         * @param p_r Red component [0, 255].
         * @param p_g Green component [0, 255].
         * @param p_b Blue component [0, 255].
         * @param p_a Alpha component [0, 255] (default is 255).
         */
        Color(int p_r, int p_g, int p_b, int p_a = 255);

        /**
         * @brief Constructor to initialize color from float values.
         *
         * @param p_r Red component [0.0f, 1.0f].
         * @param p_g Green component [0.0f, 1.0f].
         * @param p_b Blue component [0.0f, 1.0f].
         * @param p_a Alpha component [0.0f, 1.0f] (default is 1.0f).
         */
        Color(float p_r, float p_g, float p_b, float p_a = 1.0f);

        /**
         * @brief Addition operator to add two colors.
         *
         * @param p_color The color to add to the current color.
         * @return A new color resulting from the addition.
         */
        Color operator+(const Color& p_color) const;

        /**
         * @brief Subtraction operator to subtract a color from the current color.
         *
         * @param p_color The color to subtract from the current color.
         * @return A new color resulting from the subtraction.
         */
        Color operator-(const Color& p_color) const;
    };

    class Colors
    {
    public:
        static const Color red;     //<! red
        static const Color blue;    //<! blue
        static const Color green;   //<! green
        static const Color yellow;  //<! yellow
        static const Color purple;  //<! purple
        static const Color cyan;    //<! cyan
        static const Color orange;  //<! orange
        static const Color magenta; //<! magenta
        static const Color pink;    //<! punk(not a color)

        static const std::vector<Color>& values() { return _values; } //<! Some constant colors.

        static const Color black; //<! black(not a color)
        static const Color white; //<! white(not a color)
        static const Color grey;  //<! grey(not a color)

    private:
        static const std::vector<Color> _values;
    };
}
