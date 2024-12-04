#pragma once

#include "structure/math/spk_bounding_box_2d.hpp"
#include "structure/math/spk_edge_2d.hpp"
#include "structure/math/spk_vector2.hpp"

#include <ostream>
#include <set>
#include <vector>

namespace spk
{
	class Polygon2D
	{
	private:
		std::vector<spk::Vector2> _points;
		std::vector<spk::Edge2D> _edges;
		std::set<spk::Edge2D::Identifier> _edgesSet;
		BoundingBox2D _boundingBox;

		mutable float _signedArea = 0.0f;
		mutable float _perimeter = 0.0f;
		mutable spk::Vector2 _centroid;
		mutable bool _dirty = true;

		void _addEdge(const spk::Vector2 &p_a, const spk::Vector2 &p_b);
		void _invalidate();
		void _updateCache() const;
		static bool _edgesIntersect(const spk::Edge2D &p_a, const spk::Edge2D &p_b);
		static bool _isPointInside(const Polygon2D &p_poly, const spk::Vector2 &p_point);

	public:
		Polygon2D() = default;
		Polygon2D(const std::vector<spk::Vector2> &p_points);

		const std::vector<spk::Vector2> &points() const;
		const std::vector<spk::Edge2D> &edges() const;
		const BoundingBox2D &boundingBox() const;

		float area() const;
		float perimeter() const;
		spk::Vector2 centroid() const;

		bool isConvex() const;
		bool contains(const spk::Vector2 &p_point) const;
		bool contains(const Polygon2D &p_polygon) const;
		bool isAdjacent(const Polygon2D &p_other) const;
		bool isOverlapping(const Polygon2D &p_other) const;
		bool isSequant(const Polygon2D &p_other) const;

		Polygon2D fuze(const Polygon2D &p_other, bool p_checkCompatibility = false) const;
		static Polygon2D fuzeGroup(const std::vector<Polygon2D> &p_polygons);

		std::vector<Polygon2D> triangulate() const;
		std::vector<Polygon2D> splitIntoConvex() const;

		static Polygon2D fromLoop(const std::vector<spk::Vector2> &p_points);
		static Polygon2D makeTriangle(const spk::Vector2 &p_a, const spk::Vector2 &p_b, const spk::Vector2 &p_c);
		static Polygon2D makeSquare(const spk::Vector2 &p_a, const spk::Vector2 &p_b, const spk::Vector2 &p_c, const spk::Vector2 &p_d);

		friend std::ostream &operator<<(std::ostream &p_os, const Polygon2D &p_poly);
		friend std::wostream &operator<<(std::wostream &p_wos, const Polygon2D &p_poly);
	};
}
