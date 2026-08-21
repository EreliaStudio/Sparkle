#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "vector2.hpp"

namespace spk
{
	struct Rect2D
	{
		using Anchor = Vector2Int;
		using Size = Vector2UInt;

		union {
			Anchor anchor{};
			struct
			{
				Anchor::value_type x;
				Anchor::value_type y;
			};
		};

		union {
			Size size{};
			struct
			{
				Size::value_type width;
				Size::value_type height;
			};
		};

		Rect2D atOrigin() const;
		bool contains(const Vector2Int &point) const;
		Rect2D shrink(const Vector2Int &offset) const;
		Rect2D intersect(const Rect2D &other) const;

		bool operator==(const Rect2D &other) const noexcept;

		friend std::ostream &operator<<(std::ostream &os, const Rect2D &rect)
		{
			os << rect.anchor << " - " << rect.size;
			return os;
		}
	};
}
