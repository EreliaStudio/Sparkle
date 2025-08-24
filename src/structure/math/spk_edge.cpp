#include "structure/math/spk_edge.hpp"

#include <utility>

namespace spk
{
	Edge::Edge(const spk::Vector3 &p_a, const spk::Vector3 &p_b) :
		a(p_a),
		b(p_b)
	{
	}

	Edge Edge::inverse() const
	{
		return Edge(b, a);
	}

	bool Edge::isInverse(const Edge &p_other) const
	{
		return (a == p_other.b) && (b == p_other.a);
	}

	bool Edge::colinear(const Edge &p_other) const
	{
		spk::Vector3 u = b - a;
		spk::Vector3 v = p_other.b - p_other.a;
		return u.cross(v).norm() <= spk::Constants::pointPrecision;
	}

	bool Edge::operator==(const Edge &p_other) const
	{
		return (a == p_other.a) && (b == p_other.b);
	}

	bool Edge::operator<(const Edge &p_other) const
	{
		if (a.x < p_other.a.x)
		{
			return true;
		}
		if (a.x > p_other.a.x)
		{
			return false;
		}
		if (a.y < p_other.a.y)
		{
			return true;
		}
		if (a.y > p_other.a.y)
		{
			return false;
		}
		if (a.z < p_other.a.z)
		{
			return true;
		}
		if (a.z > p_other.a.z)
		{
			return false;
		}
		if (b.x < p_other.b.x)
		{
			return true;
		}
		if (b.x > p_other.b.x)
		{
			return false;
		}
		if (b.y < p_other.b.y)
		{
			return true;
		}
		if (b.y > p_other.b.y)
		{
			return false;
		}
		return b.z < p_other.b.z;
	}
}

namespace std
{
	size_t hash<spk::Edge>::operator()(const spk::Edge &p_edge) const noexcept
	{
		size_t h1 = std::hash<float>()(p_edge.a.x);
		size_t h2 = std::hash<float>()(p_edge.a.y);
		size_t h3 = std::hash<float>()(p_edge.a.z);
		size_t h4 = std::hash<float>()(p_edge.b.x);
		size_t h5 = std::hash<float>()(p_edge.b.y);
		size_t h6 = std::hash<float>()(p_edge.b.z);
		size_t seed = h1;
		seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= h5 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= h6 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
}
