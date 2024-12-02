#include "structure/graphics/spk_geometry_2D.hpp"

namespace spk
{
	Geometry2D::Geometry2D() :
		anchor(0, 0),
		size(0, 0)
	{

	}

	Geometry2D::Geometry2D(const Point& p_anchor, const Size& p_size) :
		anchor(p_anchor),
		size(p_size)
	{

	}

	Geometry2D::Geometry2D(const Point& p_anchor, size_t p_width, size_t p_heigth) :
		anchor(p_anchor),
		size(p_width, p_heigth)
	{

	}

	Geometry2D::Geometry2D(int p_x, int p_y, const Size& p_size) :
		anchor(p_x, p_y),
		size(p_size)
	{

	}

	Geometry2D::Geometry2D(int p_x, int p_y, size_t p_width, size_t p_heigth) :
		anchor(p_x, p_y),
		size(p_width, p_heigth)
	{

	}

	bool Geometry2D::contains(const Point& p_point) const
	{
		return (p_point.x >= anchor.x) &&
			(p_point.y >= anchor.y) &&
			(p_point.x < anchor.x + static_cast<int32_t>(size.x)) &&
			(p_point.y < anchor.y + static_cast<int32_t>(size.y));
	}

	std::wostream& operator<<(std::wostream& os, const Geometry2D& p_geometry)
	{
		os << L"Anchor : " << p_geometry.anchor << L" - Size : " << p_geometry.size;
		return os;
	}

	bool Geometry2D::operator==(const Geometry2D& p_other) const
	{
		return anchor == p_other.anchor && size == p_other.size;
	}

	bool Geometry2D::operator!=(const Geometry2D& p_other) const
	{
		return !(*this == p_other);
	}
}
