#pragma once

#include <iostream>

namespace spk
{
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
