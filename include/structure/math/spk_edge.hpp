#pragma once

#include "structure/math/spk_vector3.hpp"

namespace spk
{
	struct Edge
	{
		spk::Vector3 first;
		spk::Vector3 second;

		Edge() = default;
		Edge(const spk::Vector3 &p_first, const spk::Vector3 &p_second);

		Edge inverse() const;
		bool isInverse(const Edge &p_other) const;
		bool colinear(const Edge &p_other) const;

		bool operator==(const Edge &p_other) const;
		bool operator<(const Edge &p_other) const;
	};
}

template <>
struct std::hash<spk::Edge>
{
	size_t operator()(const spk::Edge &p_edge) const noexcept;
};
