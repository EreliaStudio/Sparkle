#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <unordered_map>

#include "structure/math/spk_math.hpp"
#include "structure/math/spk_vector2.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	template <typename TType>
	struct IVector3
	{
		static_assert(std::is_arithmetic<TType>::value, "TType must be an arithmetic type.");

		TType x, y, z;

		// Default constructor
		IVector3() :
			x(0),
			y(0),
			z(0)
		{
		}

		// Constructor with p_scalar
		template <typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
		IVector3(UType p_scalar) :
			x(static_cast<TType>(p_scalar)),
			y(static_cast<TType>(p_scalar)),
			z(static_cast<TType>(p_scalar))
		{
		}

		// Constructor with one vector2 and one p_scalar
		template <typename UType, typename VType, typename = std::enable_if_t<std::is_arithmetic<UType>::value && std::is_arithmetic<VType>::value>>
		IVector3(const spk::IVector2<UType> &p_vec2, const VType &p_scalar) :
			x(static_cast<TType>(p_vec2.x)),
			y(static_cast<TType>(p_vec2.y)),
			z(static_cast<TType>(p_scalar))
		{
		}

		// Constructor with one vector2 and one p_scalar
		template <typename UType, typename VType, typename = std::enable_if_t<std::is_arithmetic<UType>::value && std::is_arithmetic<VType>::value>>
		IVector3(const UType &p_scalar, const spk::IVector2<VType> &p_vec2) :
			x(static_cast<TType>(p_scalar)),
			y(static_cast<TType>(p_vec2.x)),
			z(static_cast<TType>(p_vec2.y))
		{
		}

		// Constructor with three values
		template <
			typename UType, typename VType, typename WType,
			typename = std::enable_if_t<std::is_arithmetic<UType>::value && std::is_arithmetic<VType>::value && std::is_arithmetic<WType>::value>>
		IVector3(UType p_xVal, VType p_yVal, WType p_zVal) :
			x(static_cast<TType>(p_xVal)),
			y(static_cast<TType>(p_yVal)),
			z(static_cast<TType>(p_zVal))
		{
		}

		// Copy constructor
		template <typename UType>
		IVector3(const IVector3<UType> &p_other) :
			x(static_cast<TType>(p_other.x)),
			y(static_cast<TType>(p_other.y)),
			z(static_cast<TType>(p_other.z))
		{
		}

		template <typename UType>
		explicit operator IVector3<UType>() const
		{
			return IVector3<UType>(static_cast<UType>(x), static_cast<UType>(y), static_cast<UType>(z));
		}

		// Copy assignment operator
		template <typename UType>
		IVector3 &operator=(const IVector3<UType> &p_other)
		{
			x = static_cast<TType>(p_other.x);
			y = static_cast<TType>(p_other.y);
			z = static_cast<TType>(p_other.z);

			return (*this);
		}

		// Stream insertion operator
		friend std::wostream &operator<<(std::wostream &p_os, const IVector3 &p_vec)
		{
			p_os << L"(" << p_vec.x << L", " << p_vec.y << L", " << p_vec.z << L")";
			return p_os;
		}

		friend std::ostream &operator<<(std::ostream &p_os, const IVector3 &p_vec)
		{
			p_os << "(" << p_vec.x << ", " << p_vec.y << ", " << p_vec.z << ")";
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
		bool operator==(const IVector3<UType> &p_other) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				constexpr TType epsilon = static_cast<TType>(1e-5);
				return std::fabs(x - static_cast<TType>(p_other.x)) < epsilon && std::fabs(y - static_cast<TType>(p_other.y)) < epsilon &&
					   std::fabs(z - static_cast<TType>(p_other.z)) < epsilon;
			}
			else
			{
				return x == static_cast<TType>(p_other.x) && y == static_cast<TType>(p_other.y) && z == static_cast<TType>(p_other.z);
			}
		}

		template <typename UType>
		bool operator==(UType p_scalar) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				constexpr TType epsilon = static_cast<TType>(1e-5);
				return std::fabs(x - static_cast<TType>(p_scalar)) < epsilon && std::fabs(y - static_cast<TType>(p_scalar)) < epsilon &&
					   std::fabs(z - static_cast<TType>(p_scalar)) < epsilon;
			}
			else
			{
				return x == static_cast<TType>(p_scalar) && y == static_cast<TType>(p_scalar) && z == static_cast<TType>(p_scalar);
			}
		}

		template <typename UType>
		bool operator!=(const IVector3<UType> &p_other) const
		{
			return !(*this == p_other);
		}

		template <typename UType>
		bool operator!=(UType p_scalar) const
		{
			return !(*this == p_scalar);
		}

		template <typename UType>
		bool operator<(const IVector3<UType> &p_other) const
		{
			return x < p_other.x || (x == p_other.x && y < p_other.y) || (x == p_other.x && y == p_other.y && z < p_other.z);
		}

		template <typename UType>
		bool operator>(const IVector3<UType> &p_other) const
		{
			return x > p_other.x || (x == p_other.x && y > p_other.y) || (x == p_other.x && y == p_other.y && z > p_other.z);
		}

		template <typename UType>
		bool operator<=(const IVector3<UType> &p_other) const
		{
			return !(*this > p_other);
		}

		template <typename UType>
		bool operator>=(const IVector3<UType> &p_other) const
		{
			return !(*this < p_other);
		}

		template <typename UType>
		bool operator<(UType p_scalar) const
		{
			return x < p_scalar && y < p_scalar && z < p_scalar;
		}

		template <typename UType>
		bool operator>(UType p_scalar) const
		{
			return x > p_scalar && y > p_scalar && z > p_scalar;
		}

		template <typename UType>
		bool operator<=(UType p_scalar) const
		{
			return x <= p_scalar && y <= p_scalar && z <= p_scalar;
		}

		template <typename UType>
		bool operator>=(UType p_scalar) const
		{
			return x >= p_scalar && y >= p_scalar && z >= p_scalar;
		}

		IVector3<TType> operator-() const
		{
			return IVector3<TType>(-x, -y, -z);
		}

		// Arithmetic operators with another vector
		template <typename UType>
		IVector3<TType> operator+(const IVector3<UType> &p_other) const
		{
			return IVector3<TType>(x + p_other.x, y + p_other.y, z + p_other.z);
		}

		template <typename UType>
		IVector3<TType> operator-(const IVector3<UType> &p_other) const
		{
			return IVector3<TType>(x - p_other.x, y - p_other.y, z - p_other.z);
		}

		template <typename UType>
		IVector3<TType> operator*(const IVector3<UType> &p_other) const
		{
			return IVector3<TType>(x * p_other.x, y * p_other.y, z * p_other.z);
		}

		template <typename UType>
		IVector3<TType> operator/(const IVector3<UType> &p_other) const
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			return IVector3<TType>(x / p_other.x, y / p_other.y, z / p_other.z);
		}

		// Arithmetic operators with a p_scalar
		template <typename UType>
		IVector3<TType> operator+(UType p_scalar) const
		{
			return IVector3<TType>(x + p_scalar, y + p_scalar, z + p_scalar);
		}

		template <typename UType>
		IVector3<TType> operator-(UType p_scalar) const
		{
			return IVector3<TType>(x - p_scalar, y - p_scalar, z - p_scalar);
		}

		template <typename UType>
		IVector3<TType> operator*(UType p_scalar) const
		{
			return IVector3<TType>(x * p_scalar, y * p_scalar, z * p_scalar);
		}

		template <typename UType>
		IVector3<TType> operator/(UType p_scalar) const
		{
			if (p_scalar == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			return IVector3<TType>(x / p_scalar, y / p_scalar, z / p_scalar);
		}

		// Compound assignment operators with another vector
		template <typename UType>
		IVector3<TType> &operator+=(const IVector3<UType> &p_other)
		{
			x += p_other.x;
			y += p_other.y;
			z += p_other.z;
			return *this;
		}

		template <typename UType>
		IVector3<TType> &operator-=(const IVector3<UType> &p_other)
		{
			x -= p_other.x;
			y -= p_other.y;
			z -= p_other.z;
			return *this;
		}

		template <typename UType>
		IVector3<TType> &operator*=(const IVector3<UType> &p_other)
		{
			x *= p_other.x;
			y *= p_other.y;
			z *= p_other.z;
			return *this;
		}

		template <typename UType>
		IVector3<TType> &operator/=(const IVector3<UType> &p_other)
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			x /= p_other.x;
			y /= p_other.y;
			z /= p_other.z;
			return *this;
		}

		// Compound assignment operators with a p_scalar
		template <typename UType>
		IVector3<TType> &operator+=(UType p_scalar)
		{
			x += static_cast<TType>(p_scalar);
			y += static_cast<TType>(p_scalar);
			z += static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename UType>
		IVector3<TType> &operator-=(UType p_scalar)
		{
			x -= static_cast<TType>(p_scalar);
			y -= static_cast<TType>(p_scalar);
			z -= static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename UType>
		IVector3<TType> &operator*=(UType p_scalar)
		{
			x *= static_cast<TType>(p_scalar);
			y *= static_cast<TType>(p_scalar);
			z *= static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename UType>
		IVector3<TType> &operator/=(UType p_scalar)
		{
			if (p_scalar == 0)
			{
				GENERATE_ERROR("Division by zero");
			}

			x /= static_cast<TType>(p_scalar);
			y /= static_cast<TType>(p_scalar);
			z /= static_cast<TType>(p_scalar);
			return *this;
		}

		float distance(const IVector3<TType> &p_other) const
		{
			return static_cast<float>(std::sqrt(std::pow(p_other.x - x, 2) + std::pow(p_other.y - y, 2) + std::pow(p_other.z - z, 2)));
		}

		float norm() const
		{
			return static_cast<float>(std::sqrt(x * x + y * y + z * z));
		}

		IVector3<float> normalize() const
		{
			float len = norm();
			if (len == 0)
			{
				GENERATE_ERROR("Can't calculated a norm for a vector of len 0");
			}
			return IVector3<float>(x / len, y / len, z / len);
		}

		IVector3<TType> cross(const IVector3<TType> &p_other) const
		{
			return IVector3<TType>(y * p_other.z - z * p_other.y, z * p_other.x - x * p_other.z, x * p_other.y - y * p_other.x);
		}

		IVector3<TType> rotate(const IVector3<TType> &p_rotationValues) const
		{
			TType cos_x = std::cos(spk::degreeToRadian(static_cast<float>(p_rotationValues.x)));
			TType sin_x = std::sin(spk::degreeToRadian(static_cast<float>(p_rotationValues.x)));
			TType cos_y = std::cos(spk::degreeToRadian(static_cast<float>(p_rotationValues.y)));
			TType sin_y = std::sin(spk::degreeToRadian(static_cast<float>(p_rotationValues.y)));
			TType cos_z = std::cos(spk::degreeToRadian(static_cast<float>(p_rotationValues.z)));
			TType sin_z = std::sin(spk::degreeToRadian(static_cast<float>(p_rotationValues.z)));

			IVector3<TType> result;
			result.x = cos_y * cos_z * x + (sin_x * sin_y * cos_z - cos_x * sin_z) * y + (cos_x * sin_y * cos_z + sin_x * sin_z) * z;
			result.y = cos_y * sin_z * x + (sin_x * sin_y * sin_z + cos_x * cos_z) * y + (cos_x * sin_y * sin_z - sin_x * cos_z) * z;
			result.z = -sin_y * x + sin_x * cos_y * y + cos_x * cos_y * z;

			return result;
		}

		TType dot(const IVector3<TType> &p_other) const
		{
			return x * p_other.x + y * p_other.y + z * p_other.z;
		}

		IVector3 inverse() const
		{
			return IVector3(-x, -y, -z);
		}

		IVector2<TType> xy() const
		{
			return IVector2<TType>(x, y);
		}

		IVector2<TType> xz() const
		{
			return IVector2<TType>(x, z);
		}

		IVector2<TType> yz() const
		{
			return IVector2<TType>(y, z);
		}

		static IVector3 radianToDegree(const IVector3 &p_radians)
		{
			return IVector3(spk::radianToDegree(p_radians.x), spk::radianToDegree(p_radians.y), spk::radianToDegree(p_radians.z));
		}

		static IVector3 degreeToRadian(const IVector3 &p_degrees)
		{
			return IVector3(spk::degreeToRadian(p_degrees.x), spk::degreeToRadian(p_degrees.y), spk::degreeToRadian(p_degrees.z));
		}

		IVector3 clamp(const IVector3 &p_lowerValue, const IVector3 &p_higherValue)
		{
			return IVector3(std::clamp(x, p_lowerValue.x, p_higherValue.x),
							std::clamp(y, p_lowerValue.y, p_higherValue.y),
							std::clamp(z, p_lowerValue.z, p_higherValue.z));
		}

		IVector3 floor() const
		{
			IVector3 result;

			result.x = std::floor(x);
			result.y = std::floor(y);
			result.z = std::floor(z);

			return result;
		}

		IVector3 ceil() const
		{
			IVector3 result;

			result.x = std::ceil(x);
			result.y = std::ceil(y);
			result.z = std::ceil(z);

			return result;
		}

		IVector3 round() const
		{
			IVector3 result;

			result.x = std::round(x);
			result.y = std::round(y);
			result.z = std::round(z);

			return result;
		}

		template <typename TOtherType>
		static IVector3 floor(const IVector3<TOtherType> &p_vector)
		{
			IVector3 result;
			result.x = std::floor(p_vector.x);
			result.y = std::floor(p_vector.y);
			result.z = std::floor(p_vector.z);
			return result;
		}

		template <typename TOtherType>
		static IVector3 ceiling(const IVector3<TOtherType> &p_vector)
		{
			IVector3 result;
			result.x = std::ceil(p_vector.x);
			result.y = std::ceil(p_vector.y);
			result.z = std::ceil(p_vector.z);
			return result;
		}

		template <typename TOtherType>
		static IVector3 round(const IVector3<TOtherType> &p_vector)
		{
			IVector3 result;
			result.x = std::round(p_vector.x);
			result.y = std::round(p_vector.y);
			result.z = std::round(p_vector.z);
			return result;
		}

		static IVector3 min(const IVector3 &p_min, const IVector3 &p_max)
		{
			return IVector3(std::min(p_min.x, p_max.x), std::min(p_min.y, p_max.y), std::min(p_min.z, p_max.z));
		}

		static IVector3 max(const IVector3 &p_min, const IVector3 &p_max)
		{
			return IVector3(std::max(p_min.x, p_max.x), std::max(p_min.y, p_max.y), std::max(p_min.z, p_max.z));
		}

		template <typename... Args>
		static IVector3 min(const IVector3 &p_valueA, const IVector3 &p_valueB, const Args &...p_args)
		{
			return min(min(p_valueA, p_valueB), p_args...);
		}

		template <typename... Args>
		static IVector3 max(const IVector3 &p_valueA, const IVector3 &p_valueB, const Args &...p_args)
		{
			return max(max(p_valueA, p_valueB), p_args...);
		}

		static IVector3 lerp(const IVector3 &p_startingPoint, const IVector3 &p_endingPoint, float t)
		{
			return IVector3(p_startingPoint.x + (p_endingPoint.x - p_startingPoint.x) * t,
							p_startingPoint.y + (p_endingPoint.y - p_startingPoint.y) * t,
							p_startingPoint.z + (p_endingPoint.z - p_startingPoint.z) * t);
		}

		IVector3 positiveModulo(const IVector3 &p_modulo) const
		{
			return (IVector3(spk::positiveModulo(x, p_modulo.x), spk::positiveModulo(y, p_modulo.y), spk::positiveModulo(z, p_modulo.z)));
		}
	};

	using Vector3 = IVector3<float_t>;
	using Vector3Int = IVector3<int32_t>;
	using Vector3UInt = IVector3<uint32_t>;
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector3<TType> operator+(UType p_scalar, const spk::IVector3<TType> &p_vec)
{
	return spk::IVector3<TType>(p_scalar + p_vec.x, p_scalar + p_vec.y, p_scalar + p_vec.z);
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector3<TType> operator-(UType p_scalar, const spk::IVector3<TType> &p_vec)
{
	return spk::IVector3<TType>(p_scalar - p_vec.x, p_scalar - p_vec.y, p_scalar - p_vec.z);
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector3<TType> operator*(UType p_scalar, const spk::IVector3<TType> &p_vec)
{
	return spk::IVector3<TType>(p_scalar * p_vec.x, p_scalar * p_vec.y, p_scalar * p_vec.z);
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector3<TType> operator/(UType p_scalar, const spk::IVector3<TType> &p_vec)
{
	if (p_vec.x == 0 || p_vec.y == 0 || p_vec.z == 0)
	{
		GENERATE_ERROR("Division by zero");
	}
	return spk::IVector3<TType>(p_scalar / p_vec.x, p_scalar / p_vec.y, p_scalar / p_vec.z);
}

namespace std
{
	template <typename TType>
	struct hash<spk::IVector3<TType>>
	{
		size_t operator()(const spk::IVector3<TType> &p_vec) const
		{
			return hash<TType>()(p_vec.x) ^ (hash<TType>()(p_vec.y) << 1) ^ (hash<TType>()(p_vec.z) << 2);
		}
	};
}
