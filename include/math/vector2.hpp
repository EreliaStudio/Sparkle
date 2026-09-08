#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "exception.hpp"

namespace spk
{

	template <typename TType>
	struct TVector2
	{
		using value_type = TType;

		union {
			struct
			{
				TType x;
				TType y;
			};

			std::array<TType, 2> data;
		};

		constexpr TVector2() noexcept :
			x{},
			y{}
		{
		}

		constexpr TVector2(TType p_x, TType p_y) noexcept :
			x(p_x),
			y(p_y)
		{
		}

		template <typename TOther>
		constexpr TVector2(const TVector2<TOther> &other) noexcept :
			x(static_cast<TType>(other.x)),
			y(static_cast<TType>(other.y))
		{
		}

		[[nodiscard]] constexpr bool operator==(const TVector2 &other) const
		{
			return x == other.x && y == other.y;
		}

		[[nodiscard]] constexpr TVector2 operator-() const
			requires std::is_signed_v<TType>
		{
			return {-x, -y};
		}

		[[nodiscard]] TVector2 operator+(const TVector2 &other) const
		{
			return {x + other.x, y + other.y};
		}

		TVector2 &operator+=(const TVector2 &other)
		{
			x += other.x;
			y += other.y;
			return *this;
		}

		[[nodiscard]] TVector2 operator-(const TVector2 &other) const
		{
			return {x - other.x, y - other.y};
		}

		TVector2 &operator-=(const TVector2 &other)
		{
			x -= other.x;
			y -= other.y;
			return *this;
		}

		[[nodiscard]] TVector2 operator*(const TVector2 &other) const
		{
			return {x * other.x, y * other.y};
		}

		TVector2 &operator*=(const TVector2 &other)
		{
			x *= other.x;
			y *= other.y;
			return *this;
		}

		[[nodiscard]] TVector2 operator/(const TVector2 &other) const
		{
			return {x / other.x, y / other.y};
		}

		TVector2 &operator/=(const TVector2 &other)
		{
			x /= other.x;
			y /= other.y;
			return *this;
		}

		[[nodiscard]] auto length() const
		{
			return std::sqrt(x * x + y * y);
		}

		[[nodiscard]] TVector2 normalized() const
			requires std::is_floating_point_v<TType>
		{
			const auto len = length();
			if (len == 0.0)
			{
				throw spk::Exception("Cannot normalize a zero-length vector");
			}
			return *this / TVector2{static_cast<float>(len), static_cast<float>(len)};
		}

		friend std::ostream &operator<<(std::ostream &os, const TVector2 &value)
		{
			os << '(' << value.x << ", " << value.y << ')';
			return os;
		}
	};

	using Vector2 = TVector2<float_t>;
	using Vector2Int = TVector2<int32_t>;
	using Vector2UInt = TVector2<uint32_t>;
}