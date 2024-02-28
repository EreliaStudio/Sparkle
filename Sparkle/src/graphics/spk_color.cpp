#include "graphics/spk_color.hpp"

#include <algorithm>

namespace spk
{
    Color::Color() :
        r(0),
        g(0),
        b(0),
        a(1)
    {
    }
    Color::Color(int p_value) :
        Color(((p_value & 0xff000000) >> 24) & 0xff,
              (p_value & 0xff0000) >> 16,
              (p_value & 0xff00) >> 8,
              p_value & 0xff)
    {
    }

    Color::Color(int p_r, int p_g, int p_b, int p_a) :
        r(static_cast<float>(p_r) / 255.0f),
        g(static_cast<float>(p_g) / 255.0f),
        b(static_cast<float>(p_b) / 255.0f),
        a(static_cast<float>(p_a) / 255.0f)
    {
    }
    Color::Color(float p_r, float p_g, float p_b, float p_a) :
        r(p_r),
        g(p_g),
        b(p_b),
        a(p_a)
    {
    }

    Color Color::operator+(const Color& p_color) const
    {
        Color result;

        result.r = std::clamp(r + p_color.r, 0.0f, 1.0f);
        result.g = std::clamp(g + p_color.g, 0.0f, 1.0f);
        result.b = std::clamp(b + p_color.b, 0.0f, 1.0f);

        return (result);
    }

    Color Color::operator-(const Color& p_color) const
    {
        Color result;

        result.r = std::clamp(r - p_color.r, 0.0f, 1.0f);
        result.g = std::clamp(g - p_color.g, 0.0f, 1.0f);
        result.b = std::clamp(b - p_color.b, 0.0f, 1.0f);

        return (result);
    }

    const spk::Color Colors::red = Color(0xFF0000FF);
    const spk::Color Colors::blue = Color(0x0000FFFF);
    const spk::Color Colors::green = Color(0x00FF00FF);
    const spk::Color Colors::yellow = Color(0xFFFF00FF);
    const spk::Color Colors::purple = Color(0x7F00FFFF);
    const spk::Color Colors::cyan = Color(0x00FFFFFF);
    const spk::Color Colors::orange = Color(0xFF8000FF);
    const spk::Color Colors::magenta = Color(0xFF007FFF);
    const spk::Color Colors::pink = Color(0xFF00FFFF);
    const spk::Color Colors::black = Color(0x000000FF);
    const spk::Color Colors::white = Color(0xFFFFFFFF);
    const spk::Color Colors::grey = Color(0x7F7F7FFF);

    const std::vector<spk::Color> Colors::values = std::vector<spk::Color>{
        red, blue, green, yellow, purple, cyan, orange, magenta, pink};
}