#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/GL.h>

#include <iostream>
#include <string>
#include <filesystem>

#define DEBUG_LINE() std::cout << __FUNCTION__ << "::" << __LINE__ << std::endl

namespace spk
{
	void throwException(const std::string& p_errorMessage);

	template <typename NumericType>
	unsigned short numberLength(NumericType p_number, unsigned short base = 10)
	{
		unsigned short result(1);

		if (base == 0)
			throwException("numberLength: Base cannot be 0");
		if (p_number < 0)
		{
			++result;
			p_number = -p_number;
		}
		while (p_number >= base)
		{
			p_number /= base;
			++result;
		}
		return (result);
	}

	std::string readFileContent(const std::filesystem::path& p_filePath);
	float degreeToRadian(float p_degrees);
    float radianToDegree(float p_radians);

	std::string to_string(const GLenum& p_type);
	void checkOpengl(const std::string& msg);

	long long getTime();

	intmax_t positiveModulo(const intmax_t& p_value, const intmax_t& p_modulo);
}