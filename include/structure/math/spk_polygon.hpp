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
		Polygon fuze(const Polygon &p_other) const;
		static Polygon fromLoop(const std::vector<spk::Vector3> &p_vs);
		static Polygon makeTriangle(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c);
		static Polygon makeSquare(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d);

		friend std::ostream &operator<<(std::ostream &p_os, const Polygon &p_poly);
		friend std::wostream &operator<<(std::wostream &p_wos, const Polygon &p_poly);
	};
}
