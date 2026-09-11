#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "math/vector2.hpp"
#include "exception.hpp"

#include "container/json/object.hpp"

namespace spk
{
	template <typename TType>
	struct TVector3
	{
		using value_type = TType;
		union {
			struct
			{
				TType x;
				TType y;
				TType z;
			};
			std::array<TType, 3> data;
		};

		constexpr TVector3() noexcept :
			x{},
			y{},
			z{}
		{
		}

		explicit TVector3(const JSON::Value &p_value) noexcept
		{
			*this = fromJSON(p_value);
		}
		constexpr TVector3(TType x, TType y, TType z) noexcept :
			x(x),
			y(y),
			z(z)
		{
		}
		template <typename TOther>
		constexpr TVector3(const TVector3<TOther> &v) noexcept :
			x(static_cast<TType>(v.x)),
			y(static_cast<TType>(v.y)),
			z(static_cast<TType>(v.z))
		{
		}
		template <typename TOther>
		constexpr TVector3(const TVector2<TOther> &v, TType z) noexcept :
			x(static_cast<TType>(v.x)),
			y(static_cast<TType>(v.y)),
			z(z)
		{
		}

		[[nodiscard]] constexpr bool operator==(const TVector3 &other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}
		[[nodiscard]] constexpr TVector3 operator-() const
			requires std::is_signed_v<TType>
		{
			return {-x, -y, -z};
		}
		[[nodiscard]] constexpr TVector3 operator+(const TVector3 &v) const
		{
			return {x + v.x, y + v.y, z + v.z};
		}
		[[nodiscard]] constexpr TVector3 operator-(const TVector3 &v) const
		{
			return {x - v.x, y - v.y, z - v.z};
		}
		[[nodiscard]] constexpr TVector3 operator*(const TVector3 &v) const
		{
			return {x * v.x, y * v.y, z * v.z};
		}
		[[nodiscard]] constexpr TVector3 operator/(const TVector3 &v) const
		{
			return {x / v.x, y / v.y, z / v.z};
		}
		constexpr TVector3 &operator+=(const TVector3 &v)
		{
			x += v.x;
			y += v.y;
			z += v.z;
			return *this;
		}
		constexpr TVector3 &operator-=(const TVector3 &v)
		{
			x -= v.x;
			y -= v.y;
			z -= v.z;
			return *this;
		}
		constexpr TVector3 &operator*=(const TVector3 &v)
		{
			x *= v.x;
			y *= v.y;
			z *= v.z;
			return *this;
		}
		constexpr TVector3 &operator/=(const TVector3 &v)
		{
			x /= v.x;
			y /= v.y;
			z /= v.z;
			return *this;
		}

		[[nodiscard]] constexpr TType dot(const TVector3 &v) const
		{
			return x * v.x + y * v.y + z * v.z;
		}
		[[nodiscard]] constexpr TVector3 cross(const TVector3 &v) const
		{
			return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
		}
		[[nodiscard]] auto length() const
		{
			return std::sqrt(static_cast<double>(dot(*this)));
		}
		[[nodiscard]] TVector3 normalized() const
			requires std::is_floating_point_v<TType>
		{
			const auto len = length();
			if (len == 0.0)
			{
				throw spk::Exception("Cannot normalize a zero-length vector");
			}
			return *this / TVector3{static_cast<float>(len), static_cast<float>(len), static_cast<float>(len)};
		}

		friend std::ostream &operator<<(std::ostream &os, const TVector3 &v)
		{
			return os << '(' << v.x << ", " << v.y << ", " << v.z << ')';
		}

		[[nodiscard]] JSON::Value toJSON() const
		{
			JSON::Value result = JSON::Value::array();

			result.pushBack(x);
			result.pushBack(y);
			result.pushBack(z);

			return result;
		}

		[[nodiscard]] static TVector3 fromJSON(const JSON::Value &p_value)
		{
			const auto &array = p_value.asArray();

			if (array.size() != 3)
			{
				throw std::runtime_error(
					"Expected 3 elements for TVector3");
			}

			return {
				array[0].as<TType>(),
				array[1].as<TType>(),
				array[2].as<TType>()
			};
		}
	};

	using Vector3 = TVector3<float>;
	using Vector3Int = TVector3<std::int32_t>;
	using Vector3UInt = TVector3<std::uint32_t>;
}
