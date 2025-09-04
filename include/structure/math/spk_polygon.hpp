#pragma once

#include "structure/math/spk_bounding_box.hpp"
#include "structure/math/spk_edge.hpp"
#include "structure/math/spk_plane.hpp"
#include "structure/math/spk_vector3.hpp"

#include <ostream>
#include <set>
#include <vector>

namespace spk
{
	class Polygon
	{
	private:
		std::vector<spk::Vector3> _points;
		std::vector<spk::Edge> _edges;
		std::set<spk::Edge::Identifier> _edgesSet;
		BoundingBox _boundingBox;
		mutable spk::Plane _plane;

		mutable bool _dirty = true;

		void _addEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b);
		static bool _edgesIntersect(const spk::Edge &p_a, const spk::Edge &p_b, const spk::Vector3 &p_normal);
		static bool _segmentPlaneIntersection(
			const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Plane &p_plane, spk::Vector3 &p_intersection);
		static bool _isPointInside(const Polygon &p_poly, const spk::Vector3 &p_point);
		void _invalidate();
		void _updateCache() const;

	public:
		const std::vector<spk::Vector3> &points() const;
		const std::vector<spk::Edge> &edges() const;
		const spk::Plane &plane() const;
		const BoundingBox &boundingBox() const;

		bool isPlanar() const;
		bool isCoplanar(const Polygon &p_other) const;
		bool isAdjacent(const Polygon &p_other) const;
		bool isConvex() const;
		bool isOverlapping(const Polygon &p_other) const;
		bool isSequant(const Polygon &p_other) const;
		bool contains(const spk::Vector3 &p_point) const;
		bool contains(const Polygon &p_polygon) const;

		Polygon fuze(const Polygon &p_other, bool p_checkCompatibility = false) const;
		static Polygon fuzeGroup(const std::vector<Polygon> &p_polygons);

		std::vector<Polygon> splitIntoConvex() const;
		std::vector<Polygon> triangulate() const;

		static Polygon fromLoop(const std::vector<spk::Vector3> &p_vs);
		static Polygon makeTriangle(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c);
		static Polygon makeSquare(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d);

		friend std::ostream &operator<<(std::ostream &p_os, const Polygon &p_poly);
		friend std::wostream &operator<<(std::wostream &p_wos, const Polygon &p_poly);
	};
}
