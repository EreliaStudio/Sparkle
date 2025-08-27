#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/math/spk_plane.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

#include "spk_constants.hpp"
#include "spk_debug_macro.hpp"

namespace spk
{
	struct Polygon
	{
		std::vector<spk::Vector3> points;

		spk::Vector3 normal() const;
		bool isPlanar() const;
		Plane plane() const;
		bool isCoplanar(const Polygon &p_polygon) const;
		bool contains(const spk::Vector3 &p_point) const;
		bool contains(const Polygon &p_polygon) const;
	};
}
