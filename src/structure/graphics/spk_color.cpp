#include "structure/graphics/spk_color.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

#include "structure/system/spk_exception.hpp"

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
		Color(((p_value & 0xff000000) >> 24) & 0xff, (p_value & 0xff0000) >> 16, (p_value & 0xff00) >> 8, p_value & 0xff)
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

	Color::Color(const JSON::Object &p_object)
	{
		p_object >> *this;
	}

	Color Color::operator+(const Color &p_color) const
	{
		Color result;

		result.r = std::clamp(r + p_color.r, 0.0f, 1.0f);
		result.g = std::clamp(g + p_color.g, 0.0f, 1.0f);
		result.b = std::clamp(b + p_color.b, 0.0f, 1.0f);

		return (result);
	}

	Color Color::operator-(const Color &p_color) const
	{
		Color result;

		result.r = std::clamp(r - p_color.r, 0.0f, 1.0f);
		result.g = std::clamp(g - p_color.g, 0.0f, 1.0f);
		result.b = std::clamp(b - p_color.b, 0.0f, 1.0f);

		return (result);
	}

	bool Color::operator==(const Color &p_color) const
	{
		return (std::abs(r - p_color.r) < std::numeric_limits<float>::epsilon()) &&
			   (std::abs(g - p_color.g) < std::numeric_limits<float>::epsilon()) &&
			   (std::abs(b - p_color.b) < std::numeric_limits<float>::epsilon()) && (std::abs(a - p_color.a) < std::numeric_limits<float>::epsilon());
	}

	std::ostream &operator<<(std::ostream &p_os, const spk::Color &p_color)
	{
		auto flags = p_os.flags();

		p_os << "0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(p_color.r * 255)
			 << static_cast<int>(p_color.g * 255) << static_cast<int>(p_color.b * 255) << static_cast<int>(p_color.a * 255);

		p_os.flags(flags);

		return p_os;
	}

	std::wostream &operator<<(std::wostream &p_os, const spk::Color &p_color)
	{
		auto flags = p_os.flags();

		p_os << "0x" << std::uppercase << std::setfill(L'0') << std::setw(2) << std::hex << static_cast<int>(p_color.r * 255)
			 << static_cast<int>(p_color.g * 255) << static_cast<int>(p_color.b * 255) << static_cast<int>(p_color.a * 255);

		p_os.flags(flags);

		return p_os;
	}

	spk::JSON::Object &operator<<(spk::JSON::Object &p_object, const spk::Color &p_color)
	{
		std::wstringstream ss;
		ss << L"0x" << std::uppercase << std::setfill(L'0') << std::setw(2) << std::hex << static_cast<int>(p_color.r * 255) << std::setw(2)
		   << static_cast<int>(p_color.g * 255) << std::setw(2) << static_cast<int>(p_color.b * 255) << std::setw(2)
		   << static_cast<int>(p_color.a * 255);

		p_object = ss.str();
		return p_object;
	}

	const spk::JSON::Object &operator>>(const spk::JSON::Object &p_object, spk::Color &p_color)
	{
		if (p_object.isUnit() == true && p_object.hold<std::wstring>())
		{
			std::wstring colorString = p_object.as<std::wstring>();
			if (colorString.length() == 10 && colorString.substr(0, 2) == L"0x")
			{
				std::wstring hexPart = colorString.substr(2);

				for (wchar_t c : hexPart)
				{
					if (iswxdigit(c) == 0)
					{
						GENERATE_ERROR("Invalid color string format: Non-hexadecimal character found");
					}
				}

				unsigned int hexValue;
				std::wstringstream ss;
				ss << std::hex << hexPart;
				ss >> hexValue;

				if (ss.fail())
				{
					GENERATE_ERROR("Invalid color string format: Conversion failed");
				}

				p_color = spk::Color(hexValue);
			}
			else
			{
				GENERATE_ERROR("Invalid color string format");
			}
		}
		else
		{
			GENERATE_ERROR("Invalid JSON object type");
		}
		return p_object;
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
}
