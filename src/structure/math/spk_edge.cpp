#include "structure/math/spk_edge.hpp"
#include "structure/math/spk_constants.hpp"
#include "structure/math/spk_vector3.hpp"

#include <cmath>
#include <ostream>

namespace spk
{
	bool Edge::Identifier::operator==(const Identifier &p_other) const
	{
		return (a == p_other.a) && (b == p_other.b);
	}

	bool Edge::Identifier::operator<(const Edge::Identifier& p_other) const
	{
		if (a != p_other.a) { return a < p_other.a; }
		return b < p_other.b;
	}

	Edge::Identifier Edge::Identifier::from(const Edge &p_edge)
	{
		if (p_edge.first() < p_edge.second())
		{
			return Identifier{p_edge.first(), p_edge.second()};
		}
		return Identifier{p_edge.second(), p_edge.first()};
	}

	size_t Edge::IdentifierHash::operator()(const Identifier &p_edge) const noexcept
	{
		size_t h1 = std::hash<spk::Vector3>()(p_edge.a);
		size_t h2 = std::hash<spk::Vector3>()(p_edge.b);
		size_t seed = h1;
		seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}

	Edge::Edge(const spk::Vector3 &p_first, const spk::Vector3 &p_second) :
		_first(p_first),
		_second(p_second)
	{
		if (_second == _first)
		{
			GENERATE_ERROR("Can't create an edge of lenght == 0");
		}

		_delta = (_second - _first);
		_direction = _delta.normalize();
	}

	const spk::Vector3 &Edge::first() const
	{
		return _first;
	}

	const spk::Vector3 &Edge::second() const
	{
		return _second;
	}

	const spk::Vector3 &Edge::delta() const
	{
		return _delta;
	}

	const spk::Vector3 &Edge::direction() const
	{
		return _direction;
	}

	float Edge::orientation(const spk::Vector3 &p_point, const spk::Vector3 &p_normal) const
	{
		return (_delta.cross(p_point - _first)).dot(p_normal);
	}

	bool Edge::contains(const spk::Vector3 &p_point, bool p_checkAlignment) const
	{
		if (p_point == _first)
		{
			return true;
		}
		const spk::Vector3 v = p_point - _first;

		if (p_checkAlignment == true && v.normalize() != _direction)
		{
			return false;
		}

		const float t = v.dot(_direction);

		const float len = _delta.dot(_direction);

		return (t >= 0) && (t <= len);
	}

	float Edge::project(const spk::Vector3 &p_point) const
	{
		return (p_point - _first).dot(_direction);
	}

	bool Edge::isInverse(const Edge &p_other) const
	{
		return (_first == p_other.second()) && (_second == p_other.first());
	}

	Edge Edge::inverse() const
	{
		return Edge(_second, _first);
	}

	bool Edge::isParallel(const Edge &p_other) const
	{
		return (direction() == p_other.direction() || direction() == p_other.direction().inverse());
	}

	bool Edge::isColinear(const Edge &p_other) const
	{
		if (isParallel(p_other) == false)
		{
			return false;
		}

		spk::Vector3 delta = (p_other._first - _first);

		if (delta == spk::Vector3(0, 0, 0))
		{
			return true;
		}

		float dot = std::fabs(delta.normalize().dot(_direction));
		return FLOAT_EQ(dot, 1.0f);
	}

	bool Edge::isSame(const Edge &p_other) const
	{
		return ((first() == p_other.first()) && (second() == p_other.second()) || (first() == p_other.second()) && (second() == p_other.first()));
	}

	bool Edge::operator==(const Edge &p_other) const
	{
		return (first() == p_other.first()) && (second() == p_other.second());
	}

	bool Edge::operator<(const Edge &p_other) const
	{
		Identifier lhs = Identifier::from(*this);
		Identifier rhs = Identifier::from(p_other);

		if (lhs.a != rhs.a)
		{
			return lhs.a < rhs.a;
		}
		return lhs.b < rhs.b;
	}

	std::ostream &operator<<(std::ostream &p_os, const Edge &p_edge)
	{
		p_os << "(" << p_edge.first() << " -> " << p_edge.second() << ")";
		return p_os;
	}

	std::wostream &operator<<(std::wostream &p_wos, const Edge &p_edge)
	{
		p_wos << L"(" << p_edge.first() << L" -> " << p_edge.second() << L")";
		return p_wos;
	}
}

size_t std::hash<spk::Edge>::operator()(const spk::Edge &p_edge) const noexcept
{
	size_t h1 = std::hash<spk::Vector3>()(p_edge.first());
	size_t h2 = std::hash<spk::Vector3>()(p_edge.second());
	size_t seed = h1;
	seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	return seed;
}
