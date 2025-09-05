#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <unordered_map>

#include "spk_constants.hpp"
#include "structure/math/spk_math.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

#include "structure/system/spk_exception.hpp"

#include "structure/container/spk_json_object.hpp"

namespace spk
{
	template <typename TType>
	struct IVector4
	{
		static_assert(std::is_arithmetic<TType>::value, "TType must be an arithmetic type.");

		TType x, y, z, w;

		IVector4() :
			x(0),
			y(0),
			z(0),
			w(0)
		{
		}

		template <typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
		IVector4(UType p_scalar) :
			x(static_cast<TType>(p_scalar)),
			y(static_cast<TType>(p_scalar)),
			z(static_cast<TType>(p_scalar)),
			w(static_cast<TType>(p_scalar))
		{
		}

		template <
			typename UxType,
			typename UyType,
			typename UzType,
			typename UwType,
			typename = std::enable_if_t<
				std::is_arithmetic<UxType>::value && std::is_arithmetic<UyType>::value && std::is_arithmetic<UzType>::value &&
				std::is_arithmetic<UwType>::value>>
		IVector4(UxType px, UyType py, UzType pz, UwType pw) :
			x(static_cast<TType>(px)),
			y(static_cast<TType>(py)),
			z(static_cast<TType>(pz)),
			w(static_cast<TType>(pw))
		{
		}

		// Constructor with one vector2 and two p_scalar
		template <
			typename UType,
			typename VType,
			typename WType,
			typename = std::enable_if_t<std::is_arithmetic<UType>::value && std::is_arithmetic<VType>::value && std::is_arithmetic<WType>::value>>
		IVector4(const spk::IVector2<UType> &p_vec2, const VType &p_scalarZ, const WType &p_scalarW) :
			x(static_cast<TType>(p_vec2.x)),
			y(static_cast<TType>(p_vec2.y)),
			z(static_cast<TType>(p_scalarZ)),
			w(static_cast<TType>(p_scalarW))
		{
		}

		// Constructor with one vector3 and one p_scalar
		template <typename UType, typename VType, typename = std::enable_if_t<std::is_arithmetic<UType>::value && std::is_arithmetic<VType>::value>>
		IVector4(const spk::IVector3<UType> &p_vec3, const VType &p_scalar) :
			x(static_cast<TType>(p_vec3.x)),
			y(static_cast<TType>(p_vec3.y)),
			z(static_cast<TType>(p_vec3.z)),
			w(static_cast<TType>(p_scalar))
		{
		}

		template <typename UType>
		IVector4(const IVector4<UType> &p_other) :
			x(static_cast<TType>(p_other.x)),
			y(static_cast<TType>(p_other.y)),
			z(static_cast<TType>(p_other.z)),
			w(static_cast<TType>(p_other.w))
		{
		}

		template <typename UType = TType, std::enable_if_t<std::is_floating_point<UType>::value, int> = 0>
		IVector4(const spk::JSON::Object &p_input) :
			x(static_cast<TType>(p_input[L"x"].as<double>())),
			y(static_cast<TType>(p_input[L"y"].as<double>())),
			z(static_cast<TType>(p_input[L"z"].as<double>())),
			w(static_cast<TType>(p_input[L"w"].as<double>()))
		{
		}

		template <typename UType = TType, std::enable_if_t<!std::is_floating_point<UType>::value, int> = 0>
		IVector4(const spk::JSON::Object &p_input)
		{
			fromJSON(p_input);
		}

		spk::JSON::Object toJSON() const
		{
			spk::JSON::Object result;

			result.addAttribute(L"X");
			result.addAttribute(L"Y");
			result.addAttribute(L"Z");
			result.addAttribute(L"W");

			if constexpr (std::is_floating_point<TType>::value)
			{
				result[L"X"] = static_cast<double>(x);
				result[L"Y"] = static_cast<double>(y);
				result[L"Z"] = static_cast<double>(z);
				result[L"W"] = static_cast<double>(w);
			}
			else
			{
				result[L"X"] = static_cast<long>(x);
				result[L"Y"] = static_cast<long>(y);
				result[L"Z"] = static_cast<long>(z);
				result[L"W"] = static_cast<long>(w);
			}

			return (result);
		}

		void fromJSON(const spk::JSON::Object &p_input)
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				x = p_input[L"X"].as<double>();
				y = p_input[L"Y"].as<double>();
				z = p_input[L"Z"].as<double>();
				w = p_input[L"W"].as<double>();
			}
			else
			{
				x = p_input[L"X"].as<long>();
				y = p_input[L"Y"].as<long>();
				z = p_input[L"Z"].as<long>();
				w = p_input[L"W"].as<long>();
			}
		}

		template <typename UType>
		explicit operator IVector4<UType>() const
		{
			return IVector4<UType>(static_cast<UType>(x), static_cast<UType>(y), static_cast<UType>(z), static_cast<UType>(w));
		}
		template <typename UType>
		IVector4 &operator=(const IVector4<UType> &p_other)
		{
			x = static_cast<TType>(p_other.x);
			y = static_cast<TType>(p_other.y);
			z = static_cast<TType>(p_other.z);
			w = static_cast<TType>(p_other.w);
			return (*this);
		}

		spk::IVector2<TType> xy() const
		{
			return (spk::IVector2<TType>(x, y));
		}

		spk::IVector3<TType> xyz() const
		{
			return (spk::IVector3<TType>(x, y, z));
		}

		friend std::wostream &operator<<(std::wostream &p_os, const IVector4 &p_vec)
		{
			p_os << L"(" << p_vec.x << L", " << p_vec.y << L", " << p_vec.z << L", " << p_vec.w << L")";
			return p_os;
		}

		friend std::ostream &operator<<(std::ostream &p_os, const IVector4 &p_vec)
		{
			p_os << "(" << p_vec.x << ", " << p_vec.y << ", " << p_vec.z << ", " << p_vec.w << ")";
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
		bool operator==(const IVector4<UType> &p_other) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				return FLOAT_EQ(x, static_cast<TType>(p_other.x)) && FLOAT_EQ(y, static_cast<TType>(p_other.y)) &&
					   FLOAT_EQ(z, static_cast<TType>(p_other.z)) && FLOAT_EQ(w, static_cast<TType>(p_other.w));
			}
			else
			{
				return x == static_cast<TType>(p_other.x) && y == static_cast<TType>(p_other.y) && z == static_cast<TType>(p_other.z) &&
					   w == static_cast<TType>(p_other.w);
			}
		}

		template <typename UType>
		bool operator==(UType p_scalar) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				return FLOAT_EQ(x, static_cast<TType>(p_scalar)) && FLOAT_EQ(y, static_cast<TType>(p_scalar)) &&
					   FLOAT_EQ(z, static_cast<TType>(p_scalar)) && FLOAT_EQ(w, static_cast<TType>(p_scalar));
			}
			else
			{
				return x == static_cast<TType>(p_scalar) && y == static_cast<TType>(p_scalar) && z == static_cast<TType>(p_scalar) &&
					   w == static_cast<TType>(p_scalar);
			}
		}

		template <typename UType>
		bool operator!=(const IVector4<UType> &p_other) const
		{
			return !(*this == p_other);
		}

		template <typename UType>
		bool operator!=(UType p_scalar) const
		{
			return !(*this == p_scalar);
		}

		template <typename UType>
		bool operator<(const IVector4<UType> &p_other) const
		{
			return x < p_other.x || (x == p_other.x && y < p_other.y) || (x == p_other.x && y == p_other.y && z < p_other.z) ||
				   (x == p_other.x && y == p_other.y && z == p_other.z && w < p_other.w);
		}

		template <typename UType>
		bool operator>(const IVector4<UType> &p_other) const
		{
			return x > p_other.x || (x == p_other.x && y > p_other.y) || (x == p_other.x && y == p_other.y && z > p_other.z) ||
				   (x == p_other.x && y == p_other.y && z == p_other.z && w > p_other.w);
		}

		template <typename UType>
		bool operator<=(const IVector4<UType> &p_other) const
		{
			return !(*this > p_other);
		}

		template <typename UType>
		bool operator>=(const IVector4<UType> &p_other) const
		{
			return !(*this < p_other);
		}

		template <typename UType>
		bool operator<(UType p_scalar) const
		{
			return x < p_scalar && y < p_scalar && z < p_scalar && w < p_scalar;
		}

		template <typename UType>
		bool operator>(UType p_scalar) const
		{
			return x > p_scalar && y > p_scalar && z > p_scalar && w > p_scalar;
		}

		template <typename UType>
		bool operator<=(UType p_scalar) const
		{
			return x <= p_scalar && y <= p_scalar && z <= p_scalar && w <= p_scalar;
		}

		template <typename UType>
		bool operator>=(UType p_scalar) const
		{
			return x >= p_scalar && y >= p_scalar && z >= p_scalar && w >= p_scalar;
		}

		IVector4<TType> operator-() const
		{
			return IVector4<TType>(-x, -y, -z, -w);
		}

		template <typename UType>
		IVector4<TType> operator+(const IVector4<UType> &p_other) const
		{
			return IVector4<TType>(x + p_other.x, y + p_other.y, z + p_other.z, w + p_other.w);
		}

		template <typename UType>
		IVector4<TType> operator-(const IVector4<UType> &p_other) const
		{
			return IVector4<TType>(x - p_other.x, y - p_other.y, z - p_other.z, w - p_other.w);
		}

		template <typename UType>
		IVector4<TType> operator*(const IVector4<UType> &p_other) const
		{
			return IVector4<TType>(x * p_other.x, y * p_other.y, z * p_other.z, w * p_other.w);
		}

		template <typename UType>
		IVector4<TType> operator/(const IVector4<UType> &p_other) const
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0 || p_other.w == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			return IVector4<TType>(x / p_other.x, y / p_other.y, z / p_other.z, w / p_other.w);
		}

		template <typename UType>
		IVector4<TType> operator+(UType p_scalar) const
		{
			return IVector4<TType>(x + p_scalar, y + p_scalar, z + p_scalar, w + p_scalar);
		}

		template <typename UType>
		IVector4<TType> operator-(UType p_scalar) const
		{
			return IVector4<TType>(x - p_scalar, y - p_scalar, z - p_scalar, w - p_scalar);
		}

		template <typename UType>
		IVector4<TType> operator*(UType p_scalar) const
		{
			return IVector4<TType>(x * p_scalar, y * p_scalar, z * p_scalar, w * p_scalar);
		}

		template <typename UType>
		IVector4<TType> operator/(UType p_scalar) const
		{
			if (p_scalar == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			return IVector4<TType>(x / p_scalar, y / p_scalar, z / p_scalar, w / p_scalar);
		}

		template <typename UType>
		IVector4<TType> &operator+=(const IVector4<UType> &p_other)
		{
			x += p_other.x;
			y += p_other.y;
			z += p_other.z;
			w += p_other.w;
			return *this;
		}

		template <typename UType>
		IVector4<TType> &operator-=(const IVector4<UType> &p_other)
		{
			x -= p_other.x;
			y -= p_other.y;
			z -= p_other.z;
			w -= p_other.w;
			return *this;
		}

		template <typename UType>
		IVector4<TType> &operator*=(const IVector4<UType> &p_other)
		{
			x *= p_other.x;
			y *= p_other.y;
			z *= p_other.z;
			w *= p_other.w;
			return *this;
		}

		template <typename UType>
		IVector4<TType> &operator/=(const IVector4<UType> &p_other)
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0 || p_other.w == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			x /= p_other.x;
			y /= p_other.y;
			z /= p_other.z;
			w /= p_other.w;
			return *this;
		}

		template <typename UType>
		IVector4<TType> &operator+=(UType p_scalar)
		{
			x += static_cast<TType>(p_scalar);
			y += static_cast<TType>(p_scalar);
			z += static_cast<TType>(p_scalar);
			w += static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename UType>
		IVector4<TType> &operator-=(UType p_scalar)
		{
			x -= static_cast<TType>(p_scalar);
			y -= static_cast<TType>(p_scalar);
			z -= static_cast<TType>(p_scalar);
			w -= static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename UType>
		IVector4<TType> &operator*=(UType p_scalar)
		{
			x *= static_cast<TType>(p_scalar);
			y *= static_cast<TType>(p_scalar);
			z *= static_cast<TType>(p_scalar);
			w *= static_cast<TType>(p_scalar);
			return *this;
		}

		template <typename UType>
		IVector4<TType> &operator/=(UType p_scalar)
		{
			if (p_scalar == 0)
			{
				GENERATE_ERROR("Division by zero");
			}
			x /= static_cast<TType>(p_scalar);
			y /= static_cast<TType>(p_scalar);
			z /= static_cast<TType>(p_scalar);
			w /= static_cast<TType>(p_scalar);
			return *this;
		}

		float distance(const IVector4<TType> &p_other) const
		{
			return static_cast<float>(
				std::sqrt(std::pow(p_other.x - x, 2) + std::pow(p_other.y - y, 2) + std::pow(p_other.z - z, 2) + std::pow(p_other.w - w, 2)));
		}

		float norm() const
		{
			return static_cast<float>(std::sqrt(x * x + y * y + z * z + w * w));
		}

		IVector4<float> normalize() const
		{
			float len = norm();
			if (len == 0)
			{
				GENERATE_ERROR("Can't calculate a norm for a vector of length 0");
			}
			return IVector4<float>(x / len, y / len, z / len, w / len);
		}

		TType dot(const IVector4<TType> &p_other) const
		{
			return x * p_other.x + y * p_other.y + z * p_other.z + w * p_other.w;
		}

		IVector4<TType> inverse() const
		{
			return IVector4(-x, -y, -z, -w);
		}

		static IVector4 radianToDegree(const IVector4 &p_radians)
		{
			return IVector4(
				spk::radianToDegree(p_radians.x),
				spk::radianToDegree(p_radians.y),
				spk::radianToDegree(p_radians.z),
				spk::radianToDegree(p_radians.w));
		}

		static IVector4 degreeToRadian(const IVector4 &p_degrees)
		{
			return IVector4(
				spk::degreeToRadian(p_degrees.x),
				spk::degreeToRadian(p_degrees.y),
				spk::degreeToRadian(p_degrees.z),
				spk::degreeToRadian(p_degrees.w));
		}

		IVector4 clamp(const IVector4 &p_lowerValue, const IVector4 &p_higherValue)
		{
			return IVector4(
				std::clamp(x, p_lowerValue.x, p_higherValue.x),
				std::clamp(y, p_lowerValue.y, p_higherValue.y),
				std::clamp(z, p_lowerValue.z, p_higherValue.z),
				std::clamp(w, p_lowerValue.w, p_higherValue.w));
		}

		IVector4 floor() const
		{
			IVector4 result;

			result.x = std::floor(x);
			result.y = std::floor(y);
			result.z = std::floor(z);
			result.w = std::floor(w);

			return result;
		}

		IVector4 ceil() const
		{
			IVector4 result;

			result.x = std::ceil(x);
			result.y = std::ceil(y);
			result.z = std::ceil(z);
			result.w = std::ceil(w);

			return result;
		}

		IVector4 round() const
		{
			IVector4 result;

			result.x = std::round(x);
			result.y = std::round(y);
			result.z = std::round(z);
			result.w = std::round(w);

			return result;
		}

		template <typename TOtherType>
		static IVector4 floor(const IVector4<TOtherType> &p_vector)
		{
			IVector4 result;
			result.x = std::floor(p_vector.x);
			result.y = std::floor(p_vector.y);
			result.z = std::floor(p_vector.z);
			result.w = std::floor(p_vector.w);
			return result;
		}

		template <typename TOtherType>
		static IVector4 ceiling(const IVector4<TOtherType> &p_vector)
		{
			IVector4 result;
			result.x = std::ceil(p_vector.x);
			result.y = std::ceil(p_vector.y);
			result.z = std::ceil(p_vector.z);
			result.w = std::ceil(p_vector.w);
			return result;
		}

		template <typename TOtherType>
		static IVector4 round(const IVector4<TOtherType> &p_vector)
		{
			IVector4 result;
			result.x = std::round(p_vector.x);
			result.y = std::round(p_vector.y);
			result.z = std::round(p_vector.z);
			result.w = std::round(p_vector.w);
			return result;
		}

		static IVector4 min(const IVector4 &p_min, const IVector4 &p_max)
		{
			return IVector4(std::min(p_min.x, p_max.x), std::min(p_min.y, p_max.y), std::min(p_min.z, p_max.z), std::min(p_min.w, p_max.w));
		}

		static IVector4 max(const IVector4 &p_min, const IVector4 &p_max)
		{
			return IVector4(std::max(p_min.x, p_max.x), std::max(p_min.y, p_max.y), std::max(p_min.z, p_max.z), std::max(p_min.w, p_max.w));
		}

		template <typename... Args>
		static IVector4 min(const IVector4 &p_valueA, const IVector4 &p_valueB, const Args &...p_args)
		{
			return min(min(p_valueA, p_valueB), p_args...);
		}

		template <typename... Args>
		static IVector4 max(const IVector4 &p_valueA, const IVector4 &p_valueB, const Args &...p_args)
		{
			return max(max(p_valueA, p_valueB), p_args...);
		}

		static IVector4 lerp(const IVector4 &p_startingPoint, const IVector4 &p_endingPoint, float t)
		{
			return IVector4(
				p_startingPoint.x + (p_endingPoint.x - p_startingPoint.x) * t,
				p_startingPoint.y + (p_endingPoint.y - p_startingPoint.y) * t,
				p_startingPoint.z + (p_endingPoint.z - p_startingPoint.z) * t,
				p_startingPoint.w + (p_endingPoint.w - p_startingPoint.w) * t);
		}

		IVector4 positiveModulo(const IVector4 &p_modulo) const
		{
			return (IVector4(
				spk::positiveModulo(x, p_modulo.x),
				spk::positiveModulo(y, p_modulo.y),
				spk::positiveModulo(z, p_modulo.z),
				spk::positiveModulo(w, p_modulo.w)));
		}
	};

	using Vector4 = IVector4<float_t>;
	using Vector4Int = IVector4<int32_t>;
	using Vector4UInt = IVector4<uint32_t>;
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector4<TType> operator+(UType p_scalar, const spk::IVector4<TType> &p_vec)
{
	return spk::IVector4<TType>(p_scalar + p_vec.x, p_scalar + p_vec.y, p_scalar + p_vec.z, p_scalar + p_vec.w);
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector4<TType> operator-(UType p_scalar, const spk::IVector4<TType> &p_vec)
{
	return spk::IVector4<TType>(p_scalar - p_vec.x, p_scalar - p_vec.y, p_scalar - p_vec.z, p_scalar - p_vec.w);
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector4<TType> operator*(UType p_scalar, const spk::IVector4<TType> &p_vec)
{
	return spk::IVector4<TType>(p_scalar * p_vec.x, p_scalar * p_vec.y, p_scalar * p_vec.z, p_scalar * p_vec.w);
}

template <typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector4<TType> operator/(UType p_scalar, const spk::IVector4<TType> &p_vec)
{
	if (p_vec.x == 0 || p_vec.y == 0 || p_vec.z == 0 || p_vec.w == 0)
	{
		GENERATE_ERROR("Division by zero");
	}
	return spk::IVector4<TType>(p_scalar / p_vec.x, p_scalar / p_vec.y, p_scalar / p_vec.z, p_scalar / p_vec.w);
}

template <typename TType>
struct std::hash<spk::IVector4<TType>>
{
	size_t operator()(const spk::IVector4<TType> &p_vec) const
	{
		return hash<TType>()(p_vec.x) ^ (hash<TType>()(p_vec.y) << 1) ^ (hash<TType>()(p_vec.z) << 2) ^ (hash<TType>()(p_vec.w) << 3);
	}
};