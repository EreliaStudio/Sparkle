#pragma once

#include <iostream>
#include "structure/spk_iostream.hpp"
#include "structure/math/spk_vector2.hpp"

namespace spk
{
	struct Geometry2D
	{
		using Point = Vector2Int;
		using Size = Vector2UInt;

		union
		{
			Vector2Int anchor;
			struct
			{
				int32_t x;
				int32_t y;
			};
		};

		union
		{
			Vector2UInt size;
			struct
			{
				uint32_t width;
				uint32_t height;
			};
		};

		Geometry2D() :
			anchor(0, 0),
			size(0, 0)
		{

		}

		Geometry2D(const Vector2Int& p_anchor, const Vector2UInt& p_size) :
			anchor(p_anchor),
			size(p_size)
		{

		}

		Geometry2D(const Vector2Int& p_anchor, size_t p_width, size_t p_height) :
			anchor(p_anchor),
			size(p_width, p_height)
		{

		}

		Geometry2D(int p_x, int p_y, const Vector2UInt& p_size) :
			anchor(p_x, p_y),
			size(p_size)
		{

		}

		Geometry2D(int p_x, int p_y, size_t p_width, size_t p_height) :
			anchor(p_x, p_y),
			size(p_width, p_height)
		{

		}

		bool contains(const Vector2Int& p_point) const
		{
			return (p_point.x >= anchor.x) &&
				(p_point.y >= anchor.y) &&
				(p_point.x < anchor.x + static_cast<int32_t>(size.x)) &&
				(p_point.y < anchor.y + static_cast<int32_t>(size.y));
		}

		Geometry2D shrink(const Vector2Int& p_offset) const
        {
			Geometry2D result;

            result.anchor = anchor + p_offset;

			result.size = spk::Vector2UInt(
				static_cast<int>(size.x) - (2 * p_offset.x),
				static_cast<int>(size.y) - (2 * p_offset.y)
			);

			return (result);
        }

		Geometry2D operator+(const Geometry2D& p_other) const
        {
            return Geometry2D(
                anchor + p_other.anchor,
                size   + p_other.size
            );
        }

        Geometry2D operator-(const Geometry2D& p_other) const
        {
            return Geometry2D(
                anchor - p_other.anchor,
                size   - p_other.size
            );
        }
		
		Geometry2D& operator+=(const Geometry2D& p_other)
		{
			anchor += p_other.anchor; 
			size   += p_other.size;
			return *this;
		}

		Geometry2D& operator-=(const Geometry2D& p_other)
		{
			anchor -= p_other.anchor;
			size   -= p_other.size;
			return *this;
		}

		friend std::wostream& operator<<(std::wostream& os, const Geometry2D& p_geometry)
		{
			os << L"Anchor : " << p_geometry.anchor << L" - Vector2UInt : " << p_geometry.size;
			return os;
		}

		bool operator==(const Geometry2D& p_other) const
		{
			return anchor == p_other.anchor && size == p_other.size;
		}

		bool operator!=(const Geometry2D& p_other) const
		{
			return !(*this == p_other);
		}
	};
}
