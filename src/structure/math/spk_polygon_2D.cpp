#include "structure/math/spk_polygon_2d.hpp"
#include "structure/math/spk_constants.hpp"
#include "structure/math/spk_edge_map_2d.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace spk
{
	void Polygon2D::_addEdge(const spk::Vector2 &p_a, const spk::Vector2 &p_b)
	{
		spk::Edge2D tmpEdge(p_a, p_b);
		spk::Edge2D::Identifier id = spk::Edge2D::Identifier::from(tmpEdge);
		if (_edgesSet.find(id) != _edgesSet.end())
		{
			return;
		}
		_edges.push_back(std::move(tmpEdge));
		_edgesSet.insert(std::move(id));
		_boundingBox.addPoint(p_a);
		_boundingBox.addPoint(p_b);
		_invalidate();
	}

	void Polygon2D::_invalidate()
	{
		_dirty = true;
	}

	void Polygon2D::_updateCache() const
	{
		if (_dirty == false)
		{
			return;
		}

		_signedArea = 0.0f;
		_perimeter = 0.0f;
		_centroid = spk::Vector2(0.0f, 0.0f);

		if (_points.size() >= 2)
		{
			for (size_t i = 0; i < _points.size(); ++i)
			{
				const spk::Vector2 &p0 = _points[i];
				const spk::Vector2 &p1 = _points[(i + 1) % _points.size()];
				float cross = p0.crossProduct(p1);
				_signedArea += cross;
				_perimeter += (p1 - p0).norm();
				_centroid += (p0 + p1) * cross;
			}
			_signedArea *= 0.5f;
			if (std::fabs(_signedArea) > spk::Constants::pointPrecision)
			{
				_centroid = _centroid / (3.0f * _signedArea);
			}
		}

		_dirty = false;
	}

	bool Polygon2D::_edgesIntersect(const spk::Edge2D &p_a, const spk::Edge2D &p_b)
	{
		float o1 = p_a.orientation(p_b.first());
		float o2 = p_a.orientation(p_b.second());
		float o3 = p_b.orientation(p_a.first());
		float o4 = p_b.orientation(p_a.second());

		if ((o1 * o2) < -spk::Constants::pointPrecision && (o3 * o4) < -spk::Constants::pointPrecision)
		{
			return true;
		}

		if (FLOAT_EQ(o1, 0.0f) == true && p_a.contains(p_b.first(), true) == true)
		{
			return true;
		}
		if (FLOAT_EQ(o2, 0.0f) == true && p_a.contains(p_b.second(), true) == true)
		{
			return true;
		}
		if (FLOAT_EQ(o3, 0.0f) == true && p_b.contains(p_a.first(), true) == true)
		{
			return true;
		}
		if (FLOAT_EQ(o4, 0.0f) == true && p_b.contains(p_a.second(), true) == true)
		{
			return true;
		}

		if (p_a.isColinear(p_b) == true)
		{
			float t1 = p_a.project(p_b.first());
			float t2 = p_a.project(p_b.second());
			float len = p_a.delta().norm();

			float lo = std::min(t1, t2);
			float hi = std::max(t1, t2);

			float overlapLo = std::max(0.0f, lo);
			float overlapHi = std::min(len, hi);

			if (overlapHi >= overlapLo - spk::Constants::pointPrecision)
			{
				return true;
			}
		}

		return false;
	}

	bool Polygon2D::_isPointInside(const Polygon2D &p_poly, const spk::Vector2 &p_point)
	{
		for (const auto &edge : p_poly.edges())
		{
			if (edge.contains(p_point, true) == true)
			{
				return true;
			}
		}

		const auto &pts = p_poly.points();
		if (pts.size() < 3)
		{
			return false;
		}

		bool inside = false;
		for (size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++)
		{
			const spk::Vector2 &vi = pts[i];
			const spk::Vector2 &vj = pts[j];

			bool condY = ((vi.y > p_point.y) != (vj.y > p_point.y));
			if (condY == true)
			{
				float xCross = vj.x + (vi.x - vj.x) * ((p_point.y - vj.y) / (vi.y - vj.y));
				if (xCross >= p_point.x - spk::Constants::pointPrecision)
				{
					inside = !inside;
				}
			}
		}

		return inside;
	}

	Polygon2D::Polygon2D(const std::vector<spk::Vector2> &p_points) :
		_points(p_points)
	{
		for (size_t i = 0; i < _points.size(); ++i)
		{
			_addEdge(_points[i], _points[(i + 1) % _points.size()]);
		}
	}

	const std::vector<spk::Vector2> &Polygon2D::points() const
	{
		return _points;
	}

	const std::vector<spk::Edge2D> &Polygon2D::edges() const
	{
		return _edges;
	}

	const BoundingBox2D &Polygon2D::boundingBox() const
	{
		return _boundingBox;
	}

	float Polygon2D::area() const
	{
		_updateCache();
		return std::fabs(_signedArea);
	}

	float Polygon2D::perimeter() const
	{
		_updateCache();
		return _perimeter;
	}

	spk::Vector2 Polygon2D::centroid() const
	{
		_updateCache();
		return _centroid;
	}

	bool Polygon2D::isConvex() const
	{
		if (_points.size() < 3)
		{
			return false;
		}

		bool hasPos = false;
		bool hasNeg = false;

		for (size_t i = 0; i < _points.size(); ++i)
		{
			const spk::Vector2 &a = _points[i];
			const spk::Vector2 &b = _points[(i + 1) % _points.size()];
			const spk::Vector2 &c = _points[(i + 2) % _points.size()];

			float cross = spk::Edge2D(a, b).orientation(c);
			if (cross > spk::Constants::angularPrecision)
			{
				hasPos = true;
			}
			else if (cross < -spk::Constants::angularPrecision)
			{
				hasNeg = true;
			}
			if ((hasPos == true) && (hasNeg == true))
			{
				return false;
			}
		}

		return true;
	}

	bool Polygon2D::contains(const spk::Vector2 &p_point) const
	{
		if (_boundingBox.contains(p_point) == false)
		{
			return false;
		}

		return _isPointInside(*this, p_point);
	}

	bool Polygon2D::contains(const Polygon2D &p_polygon) const
	{
		for (const auto &pt : p_polygon.points())
		{
			if (contains(pt) == false)
			{
				return false;
			}
		}
		return true;
	}

	bool Polygon2D::isAdjacent(const Polygon2D &p_other) const
	{
		for (const auto &edgeA : _edges)
		{
			for (const auto &edgeB : p_other.edges())
			{
				if (edgeA.isSame(edgeB) == true)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool Polygon2D::isOverlapping(const Polygon2D &p_other) const
	{
		if (_points.size() < 3 || p_other.points().size() < 3)
		{
			return false;
		}

		if (_boundingBox.intersect(p_other.boundingBox()) == false)
		{
			return false;
		}

		for (const auto &ea : _edges)
		{
			for (const auto &eb : p_other.edges())
			{
				if (_edgesIntersect(ea, eb) == true)
				{
					return true;
				}
			}
		}

		if (contains(p_other.points().front()) == true)
		{
			return true;
		}
		if (p_other.contains(_points.front()) == true)
		{
			return true;
		}

		return false;
	}

	bool Polygon2D::isSequant(const Polygon2D &p_other) const
	{
		if (isOverlapping(p_other) == true)
		{
			return true;
		}

		return false;
	}

	Polygon2D Polygon2D::fuze(const Polygon2D &p_other, bool p_checkCompatibility) const
	{
		if (p_checkCompatibility == true && isAdjacent(p_other) == false && isOverlapping(p_other) == false)
		{
			return Polygon2D();
		}

		EdgeMap2D map;
		map.addPolygon(*this);
		map.addPolygon(p_other);
		auto polys = map.construct();
		if (polys.empty() == true)
		{
			return Polygon2D();
		}
		return polys.front();
	}

	Polygon2D Polygon2D::fuzeGroup(const std::vector<Polygon2D> &p_polygons)
	{
		EdgeMap2D map;
		for (const auto &poly : p_polygons)
		{
			map.addPolygon(poly);
		}
		auto polys = map.construct();
		if (polys.empty() == true)
		{
			return Polygon2D();
		}
		return polys.front();
	}

	std::vector<Polygon2D> Polygon2D::triangulate() const
	{
		std::vector<Polygon2D> triangles;
		if (_points.size() < 3)
		{
			return triangles;
		}

		std::vector<size_t> indices(_points.size());
		std::iota(indices.begin(), indices.end(), 0);

		while (indices.size() > 2)
		{
			bool earFound = false;
			for (size_t i = 0; i < indices.size(); ++i)
			{
				size_t prev = indices[(i + indices.size() - 1) % indices.size()];
				size_t curr = indices[i];
				size_t next = indices[(i + 1) % indices.size()];

				float cross = (_points[curr] - _points[prev]).crossProduct(_points[next] - _points[curr]);
				if (cross <= spk::Constants::angularPrecision)
				{
					continue;
				}

				Polygon2D ear = Polygon2D::makeTriangle(_points[prev], _points[curr], _points[next]);
				bool hasPoint = false;
				for (size_t j = 0; j < indices.size(); ++j)
				{
					size_t idx = indices[j];
					if (idx == prev || idx == curr || idx == next)
					{
						continue;
					}
					if (ear.contains(_points[idx]) == true)
					{
						hasPoint = true;
						break;
					}
				}
				if (hasPoint == true)
				{
					continue;
				}

				triangles.push_back(ear);
				indices.erase(indices.begin() + i);
				earFound = true;
				break;
			}
			if (earFound == false)
			{
				break;
			}
		}

		return triangles;
	}

	std::vector<Polygon2D> Polygon2D::splitIntoConvex() const
	{
		if (isConvex() == true)
		{
			return {*this};
		}
		return triangulate();
	}

	Polygon2D Polygon2D::fromLoop(const std::vector<spk::Vector2> &p_points)
	{
		return Polygon2D(p_points);
	}

	Polygon2D Polygon2D::makeTriangle(const spk::Vector2 &p_a, const spk::Vector2 &p_b, const spk::Vector2 &p_c)
	{
		return Polygon2D::fromLoop({p_a, p_b, p_c});
	}

	Polygon2D Polygon2D::makeSquare(const spk::Vector2 &p_a, const spk::Vector2 &p_b, const spk::Vector2 &p_c, const spk::Vector2 &p_d)
	{
		return Polygon2D::fromLoop({p_a, p_b, p_c, p_d});
	}

	std::ostream &operator<<(std::ostream &p_os, const Polygon2D &p_poly)
	{
		p_os << "{";
		for (size_t i = 0; i < p_poly._edges.size(); ++i)
		{
			p_os << p_poly._edges[i];
			if (i + 1 < p_poly._edges.size())
			{
				p_os << ", ";
			}
		}
		p_os << "}";
		return p_os;
	}

	std::wostream &operator<<(std::wostream &p_wos, const Polygon2D &p_poly)
	{
		p_wos << L"{";
		for (size_t i = 0; i < p_poly._edges.size(); ++i)
		{
			p_wos << p_poly._edges[i];
			if (i + 1 < p_poly._edges.size())
			{
				p_wos << L", ";
			}
		}
		p_wos << L"}";
		return p_wos;
	}
}
