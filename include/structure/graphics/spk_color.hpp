#pragma once

#include <iostream>
#include <vector>

#include "structure/container/spk_JSON_object.hpp"

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
		Color(const JSON::Object& p_object);

		Color operator+(const Color& p_color) const;
		Color operator-(const Color& p_color) const;
		bool operator==(const Color& p_color) const;

		friend std::ostream& operator << (std::ostream& p_os, const spk::Color& p_color);
		friend spk::JSON::Object& operator<<(spk::JSON::Object& p_object, const spk::Color& p_color);
		friend const spk::JSON::Object& operator>>(const spk::JSON::Object& p_object, spk::Color& p_color);

		static const Color red;
		static const Color blue;
		static const Color green;
		static const Color yellow;
		static const Color purple;
		static const Color cyan;
		static const Color orange;
		static const Color magenta;
		static const Color pink;
		static const Color black;
		static const Color white;
		static const Color grey;
	};
}
