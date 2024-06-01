#include "graphics/spk_color.hpp"

#include <algorithm>
#include <iomanip>

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

	Color::Color(const JSON::Object& p_object)
	{
		if (p_object.isObject() == true)
		{
			r = static_cast<float>(p_object.contains("r") == true ? p_object["r"].as<double>() : 0.0);
			g = static_cast<float>(p_object.contains("g") == true ? p_object["g"].as<double>() : 0.0);
			b = static_cast<float>(p_object.contains("b") == true ? p_object["b"].as<double>() : 0.0);
			a = static_cast<float>(p_object.contains("a") == true ? p_object["a"].as<double>() : 1.0);
		}
		else if (p_object.isArray() == true)
		{
			r = static_cast<float>(p_object.size() >= 1 ? p_object[0].as<double>() : 0.0);
			g = static_cast<float>(p_object.size() >= 2 ? p_object[1].as<double>() : 0.0);
			b = static_cast<float>(p_object.size() >= 3 ? p_object[2].as<double>() : 0.0);
			a = static_cast<float>(p_object.size() >= 4 ? p_object[3].as<double>() : 1.0);
		}
		else if (p_object.isUnit() == true && p_object.hold<std::string>() == true)
		{
			std::string colorString = p_object.as<std::string>();
			if (colorString.length() == 10 && colorString.substr(0, 2) == "0x")
			{
				unsigned int hexValue;
				std::stringstream ss;
				ss << std::hex << colorString.substr(2);
				ss >> hexValue;
				*this = Color(hexValue);
			}
			else
			{
				spk::throwException("Invalid color string format");
			}
		}
		else
		{
			spk::throwException("Unexpected JSON type while creating a Color");
		}
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
	
	std::ostream& operator << (std::ostream& p_os, const spk::Color& p_color)
	{
		auto flags = p_os.flags();

		p_os << "0x"
			 << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
			 << static_cast<int>(p_color.r * 255)
			 << static_cast<int>(p_color.g * 255)
			 << static_cast<int>(p_color.b * 255)
			 << static_cast<int>(p_color.a * 255);

		p_os.flags(flags);

		return p_os;
	}

	const spk::Color Color::red = Color(0xFF0000FF);
	const spk::Color Color::blue = Color(0x0000FFFF);
	const spk::Color Color::green = Color(0x00FF00FF);
	const spk::Color Color::yellow = Color(0xFFFF00FF);
	const spk::Color Color::purple = Color(0x7F00FFFF);
	const spk::Color Color::cyan = Color(0x00FFFFFF);
	const spk::Color Color::orange = Color(0xFF8000FF);
	const spk::Color Color::magenta = Color(0xFF007FFF);
	const spk::Color Color::pink = Color(0xFF00FFFF);
	const spk::Color Color::black = Color(0x000000FF);
	const spk::Color Color::white = Color(0xFFFFFFFF);
	const spk::Color Color::grey = Color(0x7F7F7FFF);

	const std::vector<spk::Color> Color::values = std::vector<spk::Color>{
		red, blue, green, yellow, purple, cyan, orange, magenta, pink};
}