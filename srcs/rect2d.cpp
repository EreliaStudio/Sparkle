#include "rect2d.hpp"

#include <algorithm>

namespace spk
{
	Rect2D Rect2D::atOrigin() const
	{
		return Rect2D{
			.anchor = {0, 0},
			.size = size
		};
	}

	bool Rect2D::contains(const Vector2Int &point) const
	{
		return (point.x >= anchor.x) && (point.y >= anchor.y) &&
			   (point.x < anchor.x + static_cast<std::int32_t>(size.x)) &&
			   (point.y < anchor.y + static_cast<std::int32_t>(size.y));
	}

	Rect2D Rect2D::shrink(const Vector2Int &offset) const
	{
		Rect2D result{
			.anchor = anchor + offset,
			.size = {0, 0}
		};

		if (size.x >= offset.x * 2 && size.y >= offset.y * 2)
		{
			result.size = Rect2D::Size(
				static_cast<std::uint32_t>(static_cast<int>(size.x) - (2 * offset.x)),
				static_cast<std::uint32_t>(static_cast<int>(size.y) - (2 * offset.y)));
		}

		return result;
	}

	Rect2D Rect2D::intersect(const Rect2D &other) const
	{
		const std::int32_t left = std::max(x, other.x);
		const std::int32_t top = std::max(y, other.y);
		const std::int32_t right = std::min(x + static_cast<std::int32_t>(width), other.x + static_cast<std::int32_t>(other.width));
		const std::int32_t bottom = std::min(y + static_cast<std::int32_t>(height), other.y + static_cast<std::int32_t>(other.height));

		if (right <= left || bottom <= top)
		{
			return Rect2D{
				.anchor = {left, top},
				.size = {0u, 0u}
			};
		}

		return Rect2D{
				.anchor = {left, top}, 
				.size = {static_cast<std::uint32_t>(right - left), static_cast<std::uint32_t>(bottom - top)}
			};
	}

	bool Rect2D::operator==(const Rect2D &other) const noexcept
	{
		return anchor == other.anchor && size == other.size;
	}
}
