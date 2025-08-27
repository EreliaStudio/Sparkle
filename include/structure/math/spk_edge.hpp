#pragma once

#include "spk_debug_macro.hpp"
#include "structure/math/spk_constants.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include <ostream>
#include <vector>

namespace spk
{
	class Edge
	{
	private:
		spk::Vector3 _first;
		spk::Vector3 _second;
		spk::Vector3 _delta;
		spk::Vector3 _direction;

	public:
		Edge(const spk::Vector3 &p_first, const spk::Vector3 &p_second);

		const spk::Vector3 &first() const;
		const spk::Vector3 &second() const;
		const spk::Vector3 &delta() const;
		const spk::Vector3 &direction() const;

		float orientation(const spk::Vector3 &p_point, const spk::Vector3 &p_normal) const;
		bool contains(const spk::Vector3 &p_point, bool p_checkAlignment = true) const;
		float project(const spk::Vector3 &p_point) const;
		bool isInverse(const Edge &p_other) const;
		Edge inverse() const;
		bool isParallel(const Edge &p_other) const;
		bool isColinear(const Edge &p_other) const;
		bool operator==(const Edge &p_other) const;
		bool isSame(const Edge &p_other) const;
		bool operator<(const Edge &p_other) const;

		friend std::ostream &operator<<(std::ostream &p_os, const Edge &p_edge);
		friend std::wostream &operator<<(std::wostream &p_wos, const Edge &p_edge);
	};
}
