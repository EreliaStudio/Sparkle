#pragma once

#include "spk_constants.hpp"
#include "structure/math/spk_vector3.hpp"
#include <functional>

namespace spk
{
	struct Edge
	{
		spk::Vector3 a;
		spk::Vector3 b;

		Edge() = default;
		Edge(const spk::Vector3 &p_a, const spk::Vector3 &p_b);

		Edge inverse() const;
		bool isInverse(const Edge &p_other) const;
		bool colinear(const Edge &p_other) const;

		bool operator==(const Edge &p_other) const;
		bool operator<(const Edge &p_other) const;
	};
}

namespace std
{
	template <>
	struct hash<spk::Edge>
	{
		size_t operator()(const spk::Edge &p_edge) const noexcept;
	};
}
