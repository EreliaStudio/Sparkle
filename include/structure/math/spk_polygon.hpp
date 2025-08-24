#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include <vector>

namespace spk
{
	struct Polygon
	{
	private:
		static float _polyScale(const std::vector<spk::Vector3> &p_points);
		static bool _approxEq(float p_a, float p_b, float p_tol);
		static bool _inRange(float p_x, float p_a, float p_b, float p_tol);
		static bool _pointOnSegment2D(const spk::Vector2 &p_p, const spk::Vector2 &p_a, const spk::Vector2 &p_b, float p_tol);

	public:
		std::vector<spk::Vector3> points;

		enum class Winding
		{
			Clockwise,
			CounterClockwise
		};

		spk::Vector3 normal() const;
		bool contains(const spk::Vector3 &p_point) const;
		bool contains(const Polygon &p_polygon) const;
		bool isCoplanar(const Polygon &p_polygon) const;
		bool isAdjacent(const Polygon &p_polygon) const;
		bool isMergable(const Polygon &p_polygon) const;
		void rewind(Winding p_winding);
		bool isConvex() const;
		std::vector<Polygon> triangulize() const;
		void merge(const Polygon &p_polygon);
		std::vector<Polygon> split() const;
	};
}
