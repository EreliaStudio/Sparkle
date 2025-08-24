#include "structure/math/spk_edge.hpp"

#include <utility>

namespace spk
{
	Edge::Edge(const spk::Vector3 &p_first, const spk::Vector3 &p_second) :
		first(p_first),
		second(p_second)
	{
	}

	Edge Edge::inverse() const
	{
		return Edge(second, first);
	}

	bool Edge::isInverse(const Edge &p_other) const
	{
		return (first == p_other.second) && (second == p_other.first);
	}

	bool Edge::colinear(const Edge &p_other) const
	{
		spk::Vector3 u = second - first;
		spk::Vector3 v = p_other.second - p_other.first;
		return u.cross(v).norm() <= std::numeric_limits<float>::epsilon();
	}

	bool Edge::operator==(const Edge &p_other) const
	{
		return (first == p_other.first) && (second == p_other.second);
	}

	bool Edge::operator<(const Edge &p_other) const
	{
		if (first != p_other.first)
		{
			return (first < p_other.first);
		}
		return (second < p_other.second);
	}
}

size_t std::hash<spk::Edge>::operator()(const spk::Edge &p_edge) const noexcept
{
	size_t h1 = std::hash<spk::Vector3>()(p_edge.first);
	size_t h2 = std::hash<spk::Vector3>()(p_edge.second);
	size_t seed = h1;
	seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	return seed;
}