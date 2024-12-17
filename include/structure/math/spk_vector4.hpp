#pragma once

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <sstream>
#include <algorithm>

#include "structure/math/spk_math.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	template<typename TType>
	struct IVector4
	{
		static_assert(std::is_arithmetic<TType>::value, "TType must be an arithmetic type.");

		TType x, y, z, w;

		IVector4() : x(0), y(0), z(0), w(0) {}
		
		template<typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
		IVector4(UType p_scalar) : x(static_cast<TType>(p_scalar)), y(static_cast<TType>(p_scalar)), z(static_cast<TType>(p_scalar)), w(static_cast<TType>(p_scalar)) {}
		
		template<typename UxType, typename UyType, typename UzType, typename UwType,
			typename = std::enable_if_t<std::is_arithmetic<UxType>::value && std::is_arithmetic<UyType>::value && std::is_arithmetic<UzType>::value && std::is_arithmetic<UwType>::value>>
		IVector4(UxType px, UyType py, UzType pz, UwType pw) : x(static_cast<TType>(px)), y(static_cast<TType>(py)), z(static_cast<TType>(pz)), w(static_cast<TType>(pw)) {}
		
		// Constructor with one vector2 and two p_scalar
        template<typename UType, typename VType, typename WType,
            typename = std::enable_if_t<std::is_arithmetic<UType>::value&& std::is_arithmetic<VType>::value&& std::is_arithmetic<WType>::value>>
        IVector4(const spk::IVector2<UType>& p_vec2, const VType& p_scalarZ, const WType& p_scalarW) : x(static_cast<TType>(p_vec2.x)), y(static_cast<TType>(p_vec2.y)), z(static_cast<TType>(p_scalarZ)), w(static_cast<TType>(p_scalarW)) {}

		// Constructor with one vector3 and one p_scalar
        template<typename UType, typename VType,
            typename = std::enable_if_t<std::is_arithmetic<UType>::value&& std::is_arithmetic<VType>::value>>
        IVector4(const spk::IVector3<UType>& p_vec3, const VType& p_scalar) : x(static_cast<TType>(p_vec3.x)), y(static_cast<TType>(p_vec3.y)), z(static_cast<TType>(p_vec3.z)), w(static_cast<TType>(p_scalar)) {}

		template <typename UType>
		IVector4(const IVector4<UType>& p_other) : x(static_cast<TType>(p_other.x)), y(static_cast<TType>(p_other.y)), z(static_cast<TType>(p_other.z)), w(static_cast<TType>(p_other.w)) {}
		
		template <typename UType>
		explicit operator IVector4<UType>() const
		{
			return IVector4<UType>(static_cast<UType>(x), static_cast<UType>(y), static_cast<UType>(z), static_cast<UType>(w));
		}
		template <typename UType>
		IVector4& operator=(const IVector4<UType>& p_other)
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

		friend std::wostream& operator<<(std::wostream& p_os, const IVector4& p_vec)
		{
			p_os << L"(" << p_vec.x << L", " << p_vec.y << L", " << p_vec.z << L", " << p_vec.w << L")";
			return p_os;
		}

		friend std::ostream& operator<<(std::ostream& p_os, const IVector4& p_vec)
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

		template<typename UType>
		bool operator==(const IVector4<UType>& p_other) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				constexpr TType epsilon = static_cast<TType>(1e-5);
				return std::fabs(x - static_cast<TType>(p_other.x)) < epsilon &&
						std::fabs(y - static_cast<TType>(p_other.y)) < epsilon &&
						std::fabs(z - static_cast<TType>(p_other.z)) < epsilon &&
						std::fabs(w - static_cast<TType>(p_other.w)) < epsilon;
			}
			else
			{
				return x == static_cast<TType>(p_other.x) && y == static_cast<TType>(p_other.y) && z == static_cast<TType>(p_other.z) && w == static_cast<TType>(p_other.w);
			}
		}

		template<typename UType>
		bool operator==(UType p_scalar) const
		{
			if constexpr (std::is_floating_point<TType>::value)
			{
				constexpr TType epsilon = static_cast<TType>(1e-5);
				return std::fabs(x - static_cast<TType>(p_scalar)) < epsilon &&
						std::fabs(y - static_cast<TType>(p_scalar)) < epsilon &&
						std::fabs(z - static_cast<TType>(p_scalar)) < epsilon &&
						std::fabs(w - static_cast<TType>(p_scalar)) < epsilon;
			}
			else
			{
				return x == static_cast<TType>(p_scalar) && y == static_cast<TType>(p_scalar) && z == static_cast<TType>(p_scalar) && w == static_cast<TType>(p_scalar);
			}
		}

		template<typename UType>
		bool operator!=(const IVector4<UType>& p_other) const
		{
			return !(*this == p_other);
		}

		template<typename UType>
		bool operator!=(UType p_scalar) const
		{
			return !(*this == p_scalar);
		}

		template<typename UType>
		bool operator<(const IVector4<UType>& p_other) const
		{
			return x < p_other.x || (x == p_other.x && y < p_other.y) || (x == p_other.x && y == p_other.y && z < p_other.z) || (x == p_other.x && y == p_other.y && z == p_other.z && w < p_other.w);
		}

		template<typename UType>
		bool operator>(const IVector4<UType>& p_other) const
		{
			return x > p_other.x || (x == p_other.x && y > p_other.y) || (x == p_other.x && y == p_other.y && z > p_other.z) || (x == p_other.x && y == p_other.y && z == p_other.z && w > p_other.w);
		}

		template<typename UType>
		bool operator<=(const IVector4<UType>& p_other) const
		{
			return !(*this > p_other);
		}

		template<typename UType>
		bool operator>=(const IVector4<UType>& p_other) const
		{
			return !(*this < p_other);
		}

		template<typename UType>
		bool operator<(UType p_scalar) const
		{
			return x < p_scalar && y < p_scalar && z < p_scalar && w < p_scalar;
		}

		template<typename UType>
		bool operator>(UType p_scalar) const
		{
			return x > p_scalar && y > p_scalar && z > p_scalar && w > p_scalar;
		}

		template<typename UType>
		bool operator<=(UType p_scalar) const
		{
			return x <= p_scalar && y <= p_scalar && z <= p_scalar && w <= p_scalar;
		}

		template<typename UType>
		bool operator>=(UType p_scalar) const
		{
			return x >= p_scalar && y >= p_scalar && z >= p_scalar && w >= p_scalar;
		}

		IVector4<TType> operator-() const
		{
			return IVector4<TType>(-x, -y, -z, -w);
		}

		template<typename UType>
		IVector4<TType> operator+(const IVector4<UType>& p_other) const
		{
			return IVector4<TType>(x + p_other.x, y + p_other.y, z + p_other.z, w + p_other.w);
		}

		template<typename UType>
		IVector4<TType> operator-(const IVector4<UType>& p_other) const
		{
			return IVector4<TType>(x - p_other.x, y - p_other.y, z - p_other.z, w - p_other.w);
		}

		template<typename UType>
		IVector4<TType> operator*(const IVector4<UType>& p_other) const
		{
			return IVector4<TType>(x * p_other.x, y * p_other.y, z * p_other.z, w * p_other.w);
		}

		template<typename UType>
		IVector4<TType> operator/(const IVector4<UType>& p_other) const
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0 || p_other.w == 0)
				throw std::runtime_error("Division by zero");
			return IVector4<TType>(x / p_other.x, y / p_other.y, z / p_other.z, w / p_other.w);
		}

		template<typename UType>
		IVector4<TType> operator+(UType p_scalar) const
		{
			return IVector4<TType>(x + p_scalar, y + p_scalar, z + p_scalar, w + p_scalar);
		}

		template<typename UType>
		IVector4<TType> operator-(UType p_scalar) const
		{
			return IVector4<TType>(x - p_scalar, y - p_scalar, z - p_scalar, w - p_scalar);
		}

		template<typename UType>
		IVector4<TType> operator*(UType p_scalar) const
		{
			return IVector4<TType>(x * p_scalar, y * p_scalar, z * p_scalar, w * p_scalar);
		}

		template<typename UType>
		IVector4<TType> operator/(UType p_scalar) const
		{
			if (p_scalar == 0)
				throw std::runtime_error("Division by zero");
			return IVector4<TType>(x / p_scalar, y / p_scalar, z / p_scalar, w / p_scalar);
		}

		template<typename UType>
		IVector4<TType>& operator+=(const IVector4<UType>& p_other)
		{
			x += p_other.x;
			y += p_other.y;
			z += p_other.z;
			w += p_other.w;
			return *this;
		}

		template<typename UType>
		IVector4<TType>& operator-=(const IVector4<UType>& p_other)
		{
			x -= p_other.x;
			y -= p_other.y;
			z -= p_other.z;
			w -= p_other.w;
			return *this;
		}

		template<typename UType>
		IVector4<TType>& operator*=(const IVector4<UType>& p_other)
		{
			x *= p_other.x;
			y *= p_other.y;
			z *= p_other.z;
			w *= p_other.w;
			return *this;
		}

		template<typename UType>
		IVector4<TType>& operator/=(const IVector4<UType>& p_other)
		{
			if (p_other.x == 0 || p_other.y == 0 || p_other.z == 0 || p_other.w == 0)
				throw std::runtime_error("Division by zero");
			x /= p_other.x;
			y /= p_other.y;
			z /= p_other.z;
			w /= p_other.w;
			return *this;
		}

		template<typename UType>
		IVector4<TType>& operator+=(UType p_scalar)
		{
			x += static_cast<TType>(p_scalar);
			y += static_cast<TType>(p_scalar);
			z += static_cast<TType>(p_scalar);
			w += static_cast<TType>(p_scalar);
			return *this;
		}

		template<typename UType>
		IVector4<TType>& operator-=(UType p_scalar)
		{
			x -= static_cast<TType>(p_scalar);
			y -= static_cast<TType>(p_scalar);
			z -= static_cast<TType>(p_scalar);
			w -= static_cast<TType>(p_scalar);
			return *this;
		}

		template<typename UType>
		IVector4<TType>& operator*=(UType p_scalar)
		{
			x *= static_cast<TType>(p_scalar);
			y *= static_cast<TType>(p_scalar);
			z *= static_cast<TType>(p_scalar);
			w *= static_cast<TType>(p_scalar);
			return *this;
		}

		template<typename UType>
		IVector4<TType>& operator/=(UType p_scalar)
		{
			if (p_scalar == 0)
				throw std::runtime_error("Division by zero");
			x /= static_cast<TType>(p_scalar);
			y /= static_cast<TType>(p_scalar);
			z /= static_cast<TType>(p_scalar);
			w /= static_cast<TType>(p_scalar);
			return *this;
		}

		float distance(const IVector4<TType>& p_other) const
		{
			return static_cast<float>(std::sqrt(std::pow(p_other.x - x, 2) + std::pow(p_other.y - y, 2) + std::pow(p_other.z - z, 2) + std::pow(p_other.w - w, 2)));
		}

		float norm() const
		{
			return static_cast<float>(std::sqrt(x * x + y * y + z * z + w * w));
		}

		IVector4<float> normalize() const
		{
			float len = norm();
			if (len == 0)
				throw std::runtime_error("Can't calculate a norm for a vector of length 0");
			return IVector4<float>(x / len, y / len, z / len, w / len);
		}

		TType dot(const IVector4<TType>& p_other) const
		{
			return x * p_other.x + y * p_other.y + z * p_other.z + w * p_other.w;
		}

		IVector4<TType> inverse() const
		{
			return IVector4(-x, -y, -z, -w);
		}

		static IVector4 radianToDegree(const IVector4& p_radians)
		{
			return IVector4(
				spk::radianToDegree(p_radians.x),
				spk::radianToDegree(p_radians.y),
				spk::radianToDegree(p_radians.z),
				spk::radianToDegree(p_radians.w)
			);
		}

		static IVector4 degreeToRadian(const IVector4& p_degrees)
		{
			return IVector4(
				spk::degreeToRadian(p_degrees.x),
				spk::degreeToRadian(p_degrees.y),
				spk::degreeToRadian(p_degrees.z),
				spk::degreeToRadian(p_degrees.w)
			);
		}

		IVector4 clamp(const IVector4& p_lowerValue, const IVector4& p_higherValue)
		{
			return IVector4(
				std::clamp(x, p_lowerValue.x, p_higherValue.x),
				std::clamp(y, p_lowerValue.y, p_higherValue.y),
				std::clamp(z, p_lowerValue.z, p_higherValue.z),
				std::clamp(w, p_lowerValue.w, p_higherValue.w)
			);
		}

		template <typename TOtherType>
		static IVector4 floor(const IVector4<TOtherType>& p_vector)
		{
			IVector4 result;
			result.x = std::floor(p_vector.x);
			result.y = std::floor(p_vector.y);
			result.z = std::floor(p_vector.z);
			result.w = std::floor(p_vector.w);
			return result;
		}

		template <typename TOtherType>
		static IVector4 ceiling(const IVector4<TOtherType>& p_vector)
		{
			IVector4 result;
			result.x = std::ceil(p_vector.x);
			result.y = std::ceil(p_vector.y);
			result.z = std::ceil(p_vector.z);
			result.w = std::ceil(p_vector.w);
			return result;
		}

		template <typename TOtherType>
		static IVector4 round(const IVector4<TOtherType>& p_vector)
		{
			IVector4 result;
			result.x = std::round(p_vector.x);
			result.y = std::round(p_vector.y);
			result.z = std::round(p_vector.z);
			result.w = std::round(p_vector.w);
			return result;
		}

		static IVector4 min(const IVector4& p_min, const IVector4& p_max)
		{
			return IVector4(std::min(p_min.x, p_max.x), std::min(p_min.y, p_max.y), std::min(p_min.z, p_max.z), std::min(p_min.w, p_max.w));
		}

		static IVector4 max(const IVector4& p_min, const IVector4& p_max)
		{
			return IVector4(std::max(p_min.x, p_max.x), std::max(p_min.y, p_max.y), std::max(p_min.z, p_max.z), std::max(p_min.w, p_max.w));
		}

		static IVector4 lerp(const IVector4& p_startingPoint, const IVector4& p_endingPoint, float t)
		{
			return IVector4(
				p_startingPoint.x + (p_endingPoint.x - p_startingPoint.x) * t,
				p_startingPoint.y + (p_endingPoint.y - p_startingPoint.y) * t,
				p_startingPoint.z + (p_endingPoint.z - p_startingPoint.z) * t,
				p_startingPoint.w + (p_endingPoint.w - p_startingPoint.w) * t
			);
		}
	};

	using Vector4 = IVector4<float_t>;
	using Vector4Int = IVector4<int32_t>;
	using Vector4UInt = IVector4<uint32_t>;
}

template<typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector4<TType> operator+(UType p_scalar, const spk::IVector4<TType>& p_vec)
{
return spk::IVector4<TType>(p_scalar + p_vec.x, p_scalar + p_vec.y, p_scalar + p_vec.z, p_scalar + p_vec.w);
}

template<typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector4<TType> operator-(UType p_scalar, const spk::IVector4<TType>& p_vec)
{
return spk::IVector4<TType>(p_scalar - p_vec.x, p_scalar - p_vec.y, p_scalar - p_vec.z, p_scalar - p_vec.w);
}

template<typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector4<TType> operator*(UType p_scalar, const spk::IVector4<TType>& p_vec)
{
return spk::IVector4<TType>(p_scalar * p_vec.x, p_scalar * p_vec.y, p_scalar * p_vec.z, p_scalar * p_vec.w);
}

template<typename TType, typename UType, typename = std::enable_if_t<std::is_arithmetic<UType>::value>>
spk::IVector4<TType> operator/(UType p_scalar, const spk::IVector4<TType>& p_vec)
{
if (p_vec.x == 0 || p_vec.y == 0 || p_vec.z == 0 || p_vec.w == 0)
	throw std::runtime_error("Division by zero");
return spk::IVector4<TType>(p_scalar / p_vec.x, p_scalar / p_vec.y, p_scalar / p_vec.z, p_scalar / p_vec.w);
}

namespace std
{
template<typename TType>
struct hash< spk::IVector4<TType> >
{
	size_t operator()(const spk::IVector4<TType>& p_vec) const
	{
		return hash<TType>()(p_vec.x) ^ (hash<TType>()(p_vec.y) << 1) ^ (hash<TType>()(p_vec.z) << 2) ^ (hash<TType>()(p_vec.w) << 3);
	}
};
}