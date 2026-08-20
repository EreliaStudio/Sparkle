#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace spk
{

	template<typename TType>
	struct TVector2
	{
		using value_type = TType;
		
		union
		{
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
		
		constexpr bool operator==(const TVector2& other) const
		{
			return x == other.x && y == other.y;
		}
		
		TVector2 operator + (const TVector2& other) const
		{
			return {x + other.x, y + other.y};
		}
		
		TVector2& operator += (const TVector2& other)
		{
			x += other.x;
			y += other.y;
			return *this;
		}
		
		TVector2 operator - (const TVector2& other) const
		{
			return {x - other.x, y - other.y};
		}
		
		TVector2& operator -= (const TVector2& other)
		{
			x -= other.x;
			y -= other.y;
			return *this;
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