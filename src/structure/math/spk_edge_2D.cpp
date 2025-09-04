#include "structure/math/spk_edge_2d.hpp"
#include "structure/math/spk_constants.hpp"

namespace
{
	float _cross2(const spk::Vector2 &a, const spk::Vector2 &b)
	{
		return (a.x * b.y - a.y * b.x);
	}

	bool _between(float v, float a, float b)
	{
		const float eps = spk::Constants::pointPrecision;
		return ((v >= std::min(a, b) - eps) == true && (v <= std::max(a, b) + eps) == true);
	}
}

namespace spk
{
	bool Edge2D::Identifier::operator==(const Identifier &p_other) const
	{
		return (a == p_other.a && b == p_other.b);
	}

	bool Edge2D::Identifier::operator<(const Identifier &p_other) const
	{
		if ((a != p_other.a) == true)
		{
			return (a < p_other.a);
		}
		return (b < p_other.b);
	}

	Edge2D::Identifier Edge2D::Identifier::from(const Edge2D &p_edge)
	{
		return {p_edge.first(), p_edge.second()};
	}

	Edge2D::Edge2D(const spk::Vector2 &p_first, const spk::Vector2 &p_second) :
		_first(p_first),
		_second(p_second),
		_delta(p_second - p_first),
		_direction()
	{
		const float len = _delta.norm();
		if (len > spk::Constants::pointPrecision)
		{
			_direction = _delta / len;
		}
		else
		{
			_direction = spk::Vector2(0.0f, 0.0f);
		}
	}

	const spk::Vector2 &Edge2D::first() const
	{
		return _first;
	}
	const spk::Vector2 &Edge2D::second() const
	{
		return _second;
	}
	const spk::Vector2 &Edge2D::delta() const
	{
		return _delta;
	}
	const spk::Vector2 &Edge2D::direction() const
	{
		return _direction;
	}

	float Edge2D::orientation(const spk::Vector2 &p_point) const
	{
		return _cross2(_delta, p_point - _first);
	}

	bool Edge2D::contains(const spk::Vector2 &p_point, bool p_checkAlignment = true) const
	{
		if (p_checkAlignment == true)
		{
			if (std::fabs(orientation(p_point)) > spk::Constants::pointPrecision)
			{
				return false;
			}
		}

		return (_between(p_point.x, _first.x, _second.x) == true && _between(p_point.y, _first.y, _second.y) == true);
	}

	float Edge2D::project(const spk::Vector2 &p_point) const
	{
		if (_direction == spk::Vector2(0.0f, 0.0f))
		{
			return 0.0f;
		}
		return (p_point - _first).dot(_direction);
	}

	bool Edge2D::isInverse(const Edge2D &p_other) const
	{
		return (_first == p_other._second && _second == p_other._first);
	}

	Edge2D Edge2D::inverse() const
	{
		return Edge2D(_second, _first);
	}

	bool Edge2D::isParallel(const Edge2D &p_other) const
	{
		const bool thisDegenerate = (_direction == spk::Vector2(0.0f, 0.0f));
		const bool otherDegenerate = (p_other._direction == spk::Vector2(0.0f, 0.0f));

		if ((thisDegenerate == true) || (otherDegenerate == true))
		{
			return (thisDegenerate == true && otherDegenerate == true);
		}

		const float c = _cross2(_direction, p_other._direction);
		return (std::fabs(c) <= spk::Constants::pointPrecision);
	}

	bool Edge2D::isColinear(const Edge2D &p_other) const
	{
		if (isParallel(p_other) == false)
		{
			return false;
		}

		const bool thisDegenerate = (_direction == spk::Vector2(0.0f, 0.0f));
		const bool otherDegenerate = (p_other._direction == spk::Vector2(0.0f, 0.0f));

		if (thisDegenerate == true && otherDegenerate == true)
		{
			return (_first == p_other._first);
		}

		const float c = _cross2(_delta, p_other._first - _first);
		return (std::fabs(c) <= spk::Constants::pointPrecision);
	}

	bool Edge2D::isSame(const Edge2D &p_other) const
	{
		return ((_first == p_other._first && _second == p_other._second) == true) ||
			   ((_first == p_other._second && _second == p_other._first) == true);
	}

	bool Edge2D::operator==(const Edge2D &p_other) const
	{
		return (_first == p_other._first && _second == p_other._second);
	}

	bool Edge2D::operator<(const Edge2D &p_other) const
	{
		if ((_first != p_other._first) == true)
		{
			return (_first < p_other._first);
		}
		return (_second < p_other._second);
	}

	std::ostream &Edge2D::operator<<(std::ostream &p_os, const Edge2D &p_edge)
	{
		p_os << "(" << p_edge._first << ", " << p_edge._second << ")";
		return p_os;
	}

	std::wostream &Edge2D::operator<<(std::wostream &p_wos, const Edge2D &p_edge)
	{
		p_wos << L"(" << p_edge._first << L", " << p_edge._second << L")";
		return p_wos;
	}
}