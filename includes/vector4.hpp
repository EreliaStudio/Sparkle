#pragma once

#include <array>
#include <cstdint>
#include <iostream>

#include "vector3.hpp"

namespace spk
{
	template <typename TType>
	struct TVector4
	{
		using value_type = TType;
		union {
			struct
			{
				TType x, y, z, w;
			};
			std::array<TType, 4> data;
		};
		constexpr TVector4() noexcept :
			x{},
			y{},
			z{},
			w{}
		{
		}
		constexpr TVector4(TType x, TType y, TType z, TType w) noexcept :
			x(x),
			y(y),
			z(z),
			w(w)
		{
		}
		template <typename T>
		constexpr TVector4(const TVector4<T> &v) noexcept :
			x(static_cast<TType>(v.x)),
			y(static_cast<TType>(v.y)),
			z(static_cast<TType>(v.z)),
			w(static_cast<TType>(v.w))
		{
		}
		template <typename T>
		constexpr TVector4(const TVector3<T> &v, TType w) noexcept :
			x(static_cast<TType>(v.x)),
			y(static_cast<TType>(v.y)),
			z(static_cast<TType>(v.z)),
			w(w)
		{
		}
		template <typename T>
		constexpr TVector4(const TVector2<T> &v, TType z, TType w) noexcept :
			x(static_cast<TType>(v.x)),
			y(static_cast<TType>(v.y)),
			z(z),
			w(w)
		{
		}
		[[nodiscard]] constexpr bool operator==(const TVector4 &) const = default;
		[[nodiscard]] constexpr TVector4 operator+(const TVector4 &v) const
		{
			return {x + v.x, y + v.y, z + v.z, w + v.w};
		}
		[[nodiscard]] constexpr TVector4 operator-(const TVector4 &v) const
		{
			return {x - v.x, y - v.y, z - v.z, w - v.w};
		}
		[[nodiscard]] constexpr TVector4 operator*(const TVector4 &v) const
		{
			return {x * v.x, y * v.y, z * v.z, w * v.w};
		}
		[[nodiscard]] constexpr TVector4 operator/(const TVector4 &v) const
		{
			return {x / v.x, y / v.y, z / v.z, w / v.w};
		}
		constexpr TVector4 &operator+=(const TVector4 &v)
		{
			x += v.x;
			y += v.y;
			z += v.z;
			w += v.w;
			return *this;
		}
		constexpr TVector4 &operator-=(const TVector4 &v)
		{
			x -= v.x;
			y -= v.y;
			z -= v.z;
			w -= v.w;
			return *this;
		}
		constexpr TVector4 &operator*=(const TVector4 &v)
		{
			x *= v.x;
			y *= v.y;
			z *= v.z;
			w *= v.w;
			return *this;
		}
		constexpr TVector4 &operator/=(const TVector4 &v)
		{
			x /= v.x;
			y /= v.y;
			z /= v.z;
			w /= v.w;
			return *this;
		}
		friend std::ostream &operator<<(std::ostream &os, const TVector4 &v)
		{
			return os << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')';
		}
	};

	using Vector4 = TVector4<float>;
	using Vector4Int = TVector4<std::int32_t>;
	using Vector4UInt = TVector4<std::uint32_t>;
}
