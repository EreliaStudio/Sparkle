#pragma once

#ifndef NOMINMAX
#	define NOMINMAX
#endif

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <unordered_map>

#include "spk_constants.hpp"
#include "structure/math/spk_math.hpp"

#include "structure/system/spk_exception.hpp"

#include "structure/container/spk_json_object.hpp"

namespace spk
{
	template <typename TType>
	struct IVector2
	{
		static_assert(std::is_arithmetic<TType>::value, "TType must be an arithmetic type.");

		TType x, y;

		// Default constructor
		IVector2() :
			x(0),
			y(0)
		{
		}

		// Constructor with p_scalar
		template <typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
		IVector2(UType p_scalar) :
			x(static_cast<TType>(p_scalar)),
			y(static_cast<TType>(p_scalar))
		{
		}

		// Constructor with two values
		template <typename UType, typename VType, typename = std::enable_if_t<std::is_arithmetic<UType>::value && std::is_arithmetic<VType>::value>>
		IVector2(UType p_xVal, VType p_yVal) :
			x(static_cast<TType>(p_xVal)),
			y(static_cast<TType>(p_yVal))
		{
		}

		// Copy constructor
		template <typename UType>
		IVector2(const IVector2<UType> &p_other) :
			x(static_cast<TType>(p_other.x)),
			y(static_cast<TType>(p_other.y))
		{
		}

		template <typename UType = TType, std::enable_if_t<std::is_floating_point<UType>::value, int> = 0>
		IVector2(const spk::JSON::Object &p_input) :
			x(static_cast<TType>(p_input[L"x"].as<double>())),
			y(static_cast<TType>(p_input[L"y"].as<double>()))
		{
		}

		template <typename UType = TType, std::enable_if_t<!std::is_floating_point<UType>::value, int> = 0>
		IVector2(const spk::JSON::Object &p_input)
		{
			fromJSON(p_input);
		}

		spk::JSON::Object toJSON() const
		{
			spk::JSON::Object result;

			result.addAttribute(L"X");
			result.addAttribute(L"Y");

			if constexpr (std::is_floating_point<TType>::value)
			{
				result[L"X"] = static_cast<double>(x);
				result[L"Y"] = static_cast<double>(y);
			}
			else
			{
				result[L"X"] = static_cast<long>(x);
				result[L"Y"] = static_cast<long>(y);
			}

			return (result);
		}

		void fromJSON(const spk::JSON::Object &p_input)
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				x = p_input[L"X"].as<double>();
				y = p_input[L"Y"].as<double>();
			}
			else
			{
				x = p_input[L"X"].as<long>();
				y = p_input[L"Y"].as<long>();
			}
		}

		template <typename UType>
		explicit operator IVector2<UType>() const
		{
			return IVector2<UType>(static_cast<UType>(x), static_cast<UType>(y));
		}

		// Copy assignment operator
		template <typename UType>
		IVector2 &operator=(const IVector2<UType> &p_other)
		{
			x = static_cast<TType>(p_other.x);
			y = static_cast<TType>(p_other.y);

			return (*this);
		}

		// Stream insertion operator
		friend std::wostream &operator<<(std::wostream &p_os, const IVector2 &p_vec)
		{
			p_os << L"(" << p_vec.x << L", " << p_vec.y << L")";
			return p_os;
		}

		friend std::ostream &operator<<(std::ostream &p_os, const IVector2 &p_vec)
		{
			p_os << "(" << p_vec.x << ", " << p_vec.y << ")";
			return p_os;
		}

		std::wstring to_wstring() const
		{
			std::wstringstream wss;
			wss << *this;
			return wss.str();
		}

		std::string to_string() const
		{
			std::stringstream ss;
			ss << *this;
			return ss.str();
		}

		template <typename UType>
		bool operator==(const IVector2<UType> &p_other) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				return FLOAT_EQ(x, static_cast<TType>(p_other.x)) && FLOAT_EQ(y, static_cast<TType>(p_other.y));
			}
			else
			{
				return x == static_cast<TType>(p_other.x) && y == static_cast<TType>(p_other.y);
			}
		}

		template <typename UType>
		bool operator==(UType p_scalar) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				return FLOAT_EQ(x, static_cast<TType>(p_scalar)) && FLOAT_EQ(y, static_cast<TType>(p_scalar));
			}
			else
			{
				return x == static_cast<TType>(p_scalar) && y == static_cast<TType>(p_scalar);
			}
		}

		template <typename UType>
		bool operator!=(const IVector2<UType> &p_other) const
		{
			return !(*this == p_other);
		}

		// Comparison operators
		template <typename UType>
		bool operator<(const IVector2<UType> &p_other) const
		{
			return x < p_other.x || (x == p_other.x && y < p_other.y);
		}

		template <typename UType>
		bool operator>(const IVector2<UType> &p_other) const
		{
			return x > p_other.x || (x == p_other.x && y > p_other.y);
		}

		template <typename UType>
		bool operator<=(const IVector2<UType> &p_other) const
		{
			return !(*this > p_other);
		}

		template <typename UType>
		bool operator>=(const IVector2<UType> &p_other) const
		{
			return !(*this < p_other);
		}

		template <typename UType>
		bool operator!=(UType p_scalar) const
		{
			return !(*this == p_scalar);
		}

		template <typename UType>
		bool operator<(UType p_scalar) const
		{
			return x < p_scalar && y < p_scalar;
		}

		template <typename UType>
		bool operator>(UType p_scalar) const
		{
			return x > p_scalar && y > p_scalar;
		}

		template <typename UType>
		bool operator<=(UType p_scalar) const
		{
			return x <= p_scalar && y <= p_scalar;
		}

		template <typename UType>
		bool operator>=(UType p_scalar) const
		{
			return x >= p_scalar && y >= p_scalar;
		}

		IVector2<TType> operator-() const
		{
			return IVector2<TType>(-x, -y);
		}

		// Arithmetic operators with another vector
		template <typename UType>
		IVector2<TType> operator+(const IVector2<UType> &p_other) const
		{
			return IVector2<TType>(x + p_other.x, y + p_other.y);
		}

		template <typename UType>
		IVector2<TType> operator-(const IVector2<UType> &p_other) const
		{
			return IVector2<TType>(x - p_other.x, y - p_other.y);
		}

		template <typename UType>
		IVector2<TType> operator*(const IVector2<UType> &p_other) const
		{
			return IVector2<TType>(x * p_other.x, y * p_other.y);
		}

		template <typename UType>
		IVector2<TType> operator/(const IVector2<UType> &p_other) const
		{
			if (p_other.x == 0 || p_other.y == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			return IVector2<TType>(x / p_other.x, y / p_other.y);
		}

		// Arithmetic operators with a p_scalar
		template <typename UType>
		IVector2<TType> operator+(UType p_scalar) const
		{
			return IVector2<TType>(x + p_scalar, y + p_scalar);
		}

		template <typename UType>
		IVector2<TType> operator-(UType p_scalar) const
		{
			return IVector2<TType>(x - p_scalar, y - p_scalar);
		}

		template <typename UType>
		IVector2<TType> operator*(UType p_scalar) const
		{
			return IVector2<TType>(x * p_scalar, y * p_scalar);
		}

		template <typename UType>
		IVector2<TType> operator/(UType p_scalar) const
		{
			if (p_scalar == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			return IVector2<TType>(x / p_scalar, y / p_scalar);
		}

		// Compound assignment operators with another vector
		template <typename UType>
		IVector2<TType> &operator+=(const IVector2<UType> &p_other)
		{
			x += p_other.x;
			y += p_other.y;
			return *this;
		}

		template <typename UType>
		IVector2<TType> &operator-=(const IVector2<UType> &p_other)
		{
			x -= p_other.x;
			y -= p_other.y;
			return *this;
		}

		template <typename UType>
		IVector2<TType> &operator*=(const IVector2<UType> &p_other)
		{
			x *= p_other.x;
			y *= p_other.y;
			return *this;
		}

		template <typename UType>
		IVector2<TType> &operator/=(const IVector2<UType> &p_other)
		{
			if (p_other.x == 0 || p_other.y == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			x /= p_other.x;
			y /= p_other.y;
			return *this;
		}

		// Compound assignment operators with a p_scalar
		template <typename UType>
		IVector2<TType> &operator+=(UType p_scalar)
		{
			x += static_cast<TType>(p_scalar);
			y += static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename UType>
		IVector2<TType> &operator-=(UType p_scalar)
		{
			x -= static_cast<TType>(p_scalar);
			y -= static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename UType>
		IVector2<TType> &operator*=(UType p_scalar)
		{
			x *= static_cast<TType>(p_scalar);
			y *= static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename UType>
		IVector2<TType> &operator/=(UType p_scalar)
		{
			if (p_scalar == 0)
			{
				GENERATE_ERROR("Division by zero");
			}

			x /= static_cast<TType>(p_scalar);
			y /= static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename TOtherType>
		float distance(const IVector2<TOtherType> &p_other) const
		{
			return static_cast<float>(std::sqrt(std::pow(p_other.x - x, 2) + std::pow(p_other.y - y, 2)));
		}

		float norm() const
		{
			return static_cast<float>(std::sqrt(x * x + y * y));
		}

		IVector2<float> normalize() const
		{
			float len = norm();
			if (len == 0)
			{
				GENERATE_ERROR("Can't calculated a norm for a vector of len 0");
			}
			return IVector2<float>(x / len, y / len);
		}

		IVector2<TType> rotate(const TType &p_angle) const
		{
			TType radian = spk::degreeToRadian(static_cast<float>(p_angle));
			TType cos_a = std::cos(radian);
			TType sin_a = std::sin(radian);

			return IVector2<TType>(x * cos_a - y * sin_a, x * sin_a + y * cos_a);
		}

		IVector2<TType> cross() const
		{
			return IVector2<TType>(-y, x);
		}

		TType crossProduct(const IVector2<TType> &p_other) const
		{
			return x * p_other.y - y * p_other.x;
		}

		TType dot(const IVector2<TType> &p_other) const
		{
			return x * p_other.x + y * p_other.y;
		}

		IVector2<TType> inverse() const
		{
			return IVector2<TType>(-x, -y);
		}

		static IVector2 radianToDegree(const IVector2 &p_radians)
		{
			return IVector2(spk::radianToDegree(p_radians.x), spk::radianToDegree(p_radians.y));
		}

		static IVector2 degreeToRadian(const IVector2 &p_degrees)
		{
			return IVector2(spk::degreeToRadian(p_degrees.x), spk::degreeToRadian(p_degrees.y));
		}

		static IVector2 clamp(const IVector2 &p_value, const IVector2 &p_lowerValue, const IVector2 &p_higherValue)
		{
			return IVector2(std::clamp(p_value.x, p_lowerValue.x, p_higherValue.x), std::clamp(p_value.y, p_lowerValue.y, p_higherValue.y));
		}

		IVector2 floor() const
		{
			IVector2 result;

			result.x = std::floor(x);
			result.y = std::floor(y);

			return result;
		}

		IVector2 ceil() const
		{
			IVector2 result;

			result.x = std::ceil(x);
			result.y = std::ceil(y);

			return result;
		}

		IVector2 round() const
		{
			IVector2 result;

			result.x = std::round(x);
			result.y = std::round(y);

			return result;
		}

		template <typename TOtherType>
		static IVector2 floor(const IVector2<TOtherType> &p_vector)
		{
			IVector2 result;
			result.x = std::floor(p_vector.x);
			result.y = std::floor(p_vector.y);
			return result;
		}

		template <typename TOtherType>
		static IVector2 ceiling(const IVector2<TOtherType> &p_vector)
		{
			IVector2 result;
			result.x = std::ceil(p_vector.x);
			result.y = std::ceil(p_vector.y);
			return result;
		}

		template <typename TOtherType>
		static IVector2 round(const IVector2<TOtherType> &p_vector)
		{
			IVector2 result;
			result.x = std::round(p_vector.x);
			result.y = std::round(p_vector.y);
			return result;
		}

		static IVector2 min(const IVector2 &p_valueA, const IVector2 &p_valueB)
		{
			return IVector2(std::min(p_valueA.x, p_valueB.x), std::min(p_valueA.y, p_valueB.y));
		}

		static IVector2 max(const IVector2 &p_valueA, const IVector2 &p_valueB)
		{
			return IVector2(std::max(p_valueA.x, p_valueB.x), std::max(p_valueA.y, p_valueB.y));
		}

		template <typename... Args>
		static IVector2 min(const IVector2 &p_valueA, const IVector2 &p_valueB, const Args &...p_args)
		{
			return min(min(p_valueA, p_valueB), p_args...);
		}

		template <typename... Args>
		static IVector2 max(const IVector2 &p_valueA, const IVector2 &p_valueB, const Args &...p_args)
		{
			return max(max(p_valueA, p_valueB), p_args...);
		}

		static IVector2 lerp(const IVector2 &p_startingPoint, const IVector2 &p_endingPoint, float t)
		{
			return IVector2(
				p_startingPoint.x + (p_endingPoint.x - p_startingPoint.x) * t, p_startingPoint.y + (p_endingPoint.y - p_startingPoint.y) * t);
		}

		IVector2 positiveModulo(const IVector2 &p_modulo) const
		{
			return (IVector2(spk::positiveModulo(x, p_modulo.x), spk::positiveModulo(y, p_modulo.y)));
		}

		bool isBetween(const IVector2 &p_min, const IVector2 &p_max)
		{
			if (x >= p_min.x && y >= p_min.y && x <= p_max.x && y <= p_max.y)
			{
				return (true);
			}
			return (false);
		}
	};

	using Vector2 = IVector2<float_t>;
	using Vector2Int = IVector2<int32_t>;
	using Vector2UInt = IVector2<uint32_t>;
}

// Arithmetic operators with p_scalar outside of the class
template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector2<TType> operator+(UType p_scalar, const spk::IVector2<TType> &p_vec)
{
	return spk::IVector2<TType>(p_scalar + p_vec.x, p_scalar + p_vec.y);
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector2<TType> operator-(UType p_scalar, const spk::IVector2<TType> &p_vec)
{
	return spk::IVector2<TType>(p_scalar - p_vec.x, p_scalar - p_vec.y);
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector2<TType> operator*(UType p_scalar, const spk::IVector2<TType> &p_vec)
{
	return spk::IVector2<TType>(p_scalar * p_vec.x, p_scalar * p_vec.y);
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector2<TType> operator/(UType p_scalar, const spk::IVector2<TType> &p_vec)
{
	if (p_vec.x == 0 || p_vec.y == 0)
	{
		GENERATE_ERROR("Division by zero");
	}
	return spk::IVector2<TType>(p_scalar / p_vec.x, p_scalar / p_vec.y);
}

template <typename TType>
struct std::hash<spk::IVector2<TType>>
{
	size_t operator()(const spk::IVector2<TType> &p_vec) const
	{
		return hash<TType>()(p_vec.x) ^ (hash<TType>()(p_vec.y) << 1);
	}
};