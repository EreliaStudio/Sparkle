#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "vector2.hpp"

namespace spk
{
	template <typename TType>
	struct TVector3
	{
		using value_type = TType;
		union {
			struct
			{
				TType x, y, z;
			};
			std::array<TType, 3> data;
		};

		constexpr TVector3() noexcept :
			x{},
			y{},
			z{}
		{
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

		[[nodiscard]] constexpr bool operator==(const TVector3 &) const = default;
		[[nodiscard]] constexpr TVector3 operator-() const
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
				throw std::domain_error("Cannot normalize a zero-length vector");
			}
			return *this / TVector3{static_cast<float>(len), static_cast<float>(len), static_cast<float>(len)};
		}

		friend std::ostream &operator<<(std::ostream &os, const TVector3 &v)
		{
			return os << '(' << v.x << ", " << v.y << ", " << v.z << ')';
		}
	};

	using Vector3 = TVector3<float>;
	using Vector3Int = TVector3<std::int32_t>;
	using Vector3UInt = TVector3<std::uint32_t>;
}
