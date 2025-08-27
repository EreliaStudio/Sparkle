#pragma once

#include "spk_debug_macro.hpp"
#include "structure/math/spk_constants.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/math/spk_edge.hpp"
#include <ostream>
#include <vector>

namespace spk
{
	class Polygon
	{
	private:
		std::vector<spk::Edge> _edges;

		void _addEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b);
		static bool _edgesIntersect(const spk::Edge &p_a, const spk::Edge &p_b, const spk::Vector3 &p_normal, float p_eps);
		static bool _isPointInside(const Polygon &p_poly, const spk::Vector3 &p_point, float p_eps);

	public:
		const std::vector<spk::Edge> &edges() const;
		std::vector<spk::Vector3> points() const;
		bool isPlanar() const;
		spk::Vector3 normal() const;
		bool isCoplanar(const Polygon &p_other) const;
		bool isAdjacent(const Polygon &p_other) const;
		bool isConvex(float p_eps = 1e-6f, bool p_strictly = false) const;
		bool isOverlapping(const Polygon &p_other) const;
		bool contains(const spk::Vector3 &p_point) const;
		bool contains(const Polygon &p_polygon) const;
		Polygon fuze(const Polygon &p_other, bool p_needVerification = true) const;
		static Polygon fromLoop(const std::vector<spk::Vector3> &p_vs);
		static Polygon makeTriangle(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c);
		static Polygon makeSquare(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d);

		friend std::ostream &operator<<(std::ostream &p_os, const Polygon &p_poly);
		friend std::wostream &operator<<(std::wostream &p_wos, const Polygon &p_poly);
	};
}
