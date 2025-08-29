#include "structure/math/spk_polygon.hpp"
#include "structure/math/spk_constants.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace spk
{
	void Polygon::_addEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b)
	{
		spk::Edge tmpEdge = spk::Edge(p_a, p_b);
		spk::Edge::Identifier id = spk::Edge::Identifier::from(tmpEdge);
		if (_edgesSet.find(id) != _edgesSet.end())
		{
			return ;	
		}

		_edges.push_back(std::move(tmpEdge));
		_edgesSet.insert(std::move(id));
		_boundingBox.addPoint(p_a);
		_boundingBox.addPoint(p_b);
		_invalidate();
	}

	void Polygon::_invalidate()
	{
		_dirty = true;
	}

	void Polygon::_updateCache() const
	{
		if (_dirty == false)
		{
			return;
		}

		if (_edges.size() < 2)
		{
			GENERATE_ERROR("Can't generate a normal for a polygon with less than 2 edges");
		}

		if (_points.empty() == false)
		{
			_plane = spk::Plane(_edges[0].direction().cross(_edges[1].direction()), _points[0]);
		}

		_dirty = false;
	}

	bool Polygon::_edgesIntersect(const spk::Edge &p_a, const spk::Edge &p_b, const spk::Vector3 &p_normal)
	{
		float o1 = p_a.orientation(p_b.first(), p_normal);
		float o2 = p_a.orientation(p_b.second(), p_normal);
		float o3 = p_b.orientation(p_a.first(), p_normal);
		float o4 = p_b.orientation(p_a.second(), p_normal);

		bool cond1 = ((o1 > spk::Constants::angularPrecision && o2 < -spk::Constants::angularPrecision) || (o1 < -spk::Constants::angularPrecision && o2 > spk::Constants::angularPrecision));
		bool cond2 = ((o3 > spk::Constants::angularPrecision && o4 < -spk::Constants::angularPrecision) || (o3 < -spk::Constants::angularPrecision && o4 > spk::Constants::angularPrecision));

		return (cond1 == true && cond2 == true);
	}

	bool Polygon::_isPointInside(const Polygon &p_poly, const spk::Vector3 &p_point)
	{
		for (const auto &edge : p_poly.edges())
		{
			if (edge.contains(p_point) == true)
			{
				return true;
			}
		}

		const auto &pts = p_poly.points();
		if (pts.size() < 3)
		{
			return false;
		}

		spk::Vector3 origin = pts[0];
		spk::Vector3 u = (pts[1] - origin).normalize();
		spk::Vector3 v = p_poly.plane().normal.cross(u);

		spk::Vector3 relP = p_point - origin;
		float px = relP.dot(u);
		float py = relP.dot(v);

		bool inside = false;

		for (size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++)
		{
			spk::Vector3 ri = pts[i] - origin;
			spk::Vector3 rj = pts[j] - origin;

			float xi = ri.dot(u);
			float yi = ri.dot(v);
			float xj = rj.dot(u);
			float yj = rj.dot(v);

			if (std::fabs(yj - yi) <= spk::Constants::angularPrecision)
			{
				continue;
			}

			bool intersects = ((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi);

			if (intersects == true)
			{
				inside = !inside;
			}
		}

		return inside;
	}

	const std::vector<spk::Vector3> &Polygon::points() const
	{
		return _points;
	}

	const std::vector<spk::Edge> &Polygon::edges() const
	{
		return _edges;
	}

	bool Polygon::isPlanar() const
	{
		if (_edges.size() < 2)
		{
			return false;
		}

		spk::Vector3 expectedNormal = _edges[0].direction().cross(_edges[1].direction()).normalize();

		for (size_t i = 2; i < _edges.size(); i++)
		{
			float dot = expectedNormal.dot(_edges[i].direction());

			if (std::fabs(dot) > spk::Constants::pointPrecision)
			{
				return false;
			}
		}
		return true;
	}

	const spk::Plane& Polygon::plane() const
	{
		if (_points.empty() == true)
		{
			GENERATE_ERROR("Can't generate a plane for a polygon without points");
		}

		if (_edges.size() < 2)
		{
			GENERATE_ERROR("Can't generate a normal for a polygon with less than 2 edges");
		}

		_updateCache();
		return _plane;
	}

	bool Polygon::isCoplanar(const Polygon &p_other) const
	{
		if (isPlanar() == false)
		{
			return false;
		}

		if (p_other.isPlanar() == false)
		{
			return false;
		}

		const spk::Plane& currentPlane = plane();
		const spk::Plane& otherPlane = p_other.plane();

		const spk::Vector3& currentNormal = currentPlane.normal.normalize();
		const spk::Vector3& otherNormal = otherPlane.normal.normalize();

		if (FLOAT_NEQ(std::fabs(currentNormal.dot(otherNormal)), 1.0f))
		{
			return false;
		}

		float currentOffset = currentNormal.dot(currentPlane.origin);
		float otherOffset = currentNormal.dot(otherPlane.origin);

		return (FLOAT_EQ(currentOffset, otherOffset) == true);
	}

	bool Polygon::isAdjacent(const Polygon &p_other) const
	{
		if (isCoplanar(p_other) == false)
		{
			return false;
		}

		if (_boundingBox.intersect(p_other._boundingBox) == false)
		{
			return false;
		}

		for (const auto &edgeIdentifier : _edgesSet)
		{
			if (p_other._edgesSet.find(edgeIdentifier) != p_other._edgesSet.end())
			{
				return true;
			}
		}

		for (const auto &edgeA : _edges)
		{
			for (const auto &edgeB : p_other.edges())
			{
				if (edgeA.isColinear(edgeB) == true)
				{
					if (edgeA.contains(edgeB.first(), false) || edgeA.contains(edgeB.second(), false) || edgeB.contains(edgeA.first(), false) ||
						edgeB.contains(edgeA.second(), false))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool Polygon::isConvex() const
	{
		if (_edges.size() < 3)
		{
			return false;
		}

		float orientation = 0;

		for (size_t i = 0; i < _edges.size(); i++)
		{
			const spk::Edge &current = _edges[i];
			const spk::Edge &next = _edges[(i + 1) % _edges.size()];

			float dot = current.direction().cross(next.direction()).dot(plane().normal);

			if (std::fabs(dot) <= spk::Constants::angularPrecision)
			{
				return false;
			}

			if (orientation == 0)
			{
				orientation = (dot > 0) ? 1.0f : -1.0f;
			}
			else if (((dot > 0) ? 1.0f : -1.0f) != orientation)
			{
				return false;
			}
		}

		return (orientation != 0);
	}

	bool Polygon::isOverlapping(const Polygon &p_other) const
	{
		if (isCoplanar(p_other) == false)
		{
			return false;
		}

		if (_boundingBox.intersect(p_other._boundingBox) == false)
		{
			return false;
		}

		for (const auto &edgeA : _edges)
		{
			for (const auto &edgeB : p_other.edges())
			{
				if (_edgesIntersect(edgeA, edgeB, plane().normal) == true)
				{
					return true;
				}
			}
		}

		if (_isPointInside(*this, p_other.edges()[0].first()) == true)
		{
			return true;
		}

		if (_isPointInside(p_other, _edges[0].first()) == true)
		{
			return true;
		}

		return false;
	}

	bool Polygon::contains(const spk::Vector3 &p_point) const
	{
		return _isPointInside(*this, p_point);
	}

	bool Polygon::contains(const Polygon &p_polygon) const
	{
		if (isCoplanar(p_polygon) == false)
		{
			return false;
		}

		for (const auto &edge : p_polygon.edges())
		{
			if (_isPointInside(*this, edge.first()) == false ||
				_isPointInside(*this, edge.second()) == false)
			{
				return false;
			}
		}
		return true;
	}

	Polygon Polygon::fromLoop(const std::vector<spk::Vector3> &p_vs)
	{
		Polygon r;
		if (p_vs.size() < 2)
		{
			return r;
		}

		std::vector<spk::Vector3> pts = p_vs;
		if (pts.front() == pts.back())
		{
			pts.pop_back();
		}

		const float tol = spk::Constants::pointPrecision;
		bool changed = true;
		while (pts.size() >= 3 && changed == true)
		{
			changed = false;
			for (size_t i = 0; i < pts.size(); ++i)
			{
				const spk::Vector3 &prev = pts[(i + pts.size() - 1) % pts.size()];
				const spk::Vector3 &curr = pts[i];
				const spk::Vector3 &next = pts[(i + 1) % pts.size()];

				spk::Vector3 v1 = curr - prev;
				spk::Vector3 v2 = next - curr;

				if (v1.cross(v2).norm() <= tol)
				{
					pts.erase(pts.begin() + i);
					changed = true;
					break;
				}
			}
		}

		r._points = pts;
		for (size_t i = 0; i < r._points.size(); ++i)
		{
			r._addEdge(r._points[i], r._points[(i + 1) % r._points.size()]);
		}

		return r;
	}

	std::vector<spk::Polygon> Polygon::splitIntoConvex() const
	{
		std::vector<spk::Polygon> result;

		if (isConvex() == true)
		{
			return {*this};
		}

		return triangulate();
	}

	std::vector<Polygon> Polygon::triangulate() const
	{
		std::vector<Polygon> triangles;
		if (_points.size() < 3)
		{
			return triangles;
		}

		std::vector<spk::Vector3> pts = _points;

		spk::Vector3 origin = pts[0];
		spk::Vector3 u = (pts[1] - origin).normalize();
		spk::Vector3 v = plane().normal.cross(u);

		std::vector<spk::Vector2> coords;
		coords.reserve(pts.size());
		for (const auto &p : pts)
		{
			spk::Vector3 rel = p - origin;
			coords.emplace_back(rel.dot(u), rel.dot(v));
		}

		std::vector<size_t> indices(pts.size());
		for (size_t i = 0; i < indices.size(); ++i)
		{
			indices[i] = i;
		}

		float area = 0.0f;
		for (size_t i = 0; i < coords.size(); ++i)
		{
			const spk::Vector2 &a = coords[i];
			const spk::Vector2 &b = coords[(i + 1) % coords.size()];
			area += a.x * b.y - a.y * b.x;
		}
		if (area < 0.0f)
		{
			std::reverse(indices.begin(), indices.end());
		}

		auto cross2D = [](const spk::Vector2 &p_a, const spk::Vector2 &p_b, const spk::Vector2 &p_c)
		{
			spk::Vector2 ab = p_b - p_a;
			spk::Vector2 ac = p_c - p_a;
			return ab.x * ac.y - ab.y * ac.x;
		};

		auto pointInTriangle = [&](const spk::Vector2 &p_p, const spk::Vector2 &p_a, const spk::Vector2 &p_b, const spk::Vector2 &p_c)
		{
			auto sign = [](const spk::Vector2 &p_p1, const spk::Vector2 &p_p2, const spk::Vector2 &p_p3)
			{ return (p_p1.x - p_p3.x) * (p_p2.y - p_p3.y) - (p_p2.x - p_p3.x) * (p_p1.y - p_p3.y); };
			float d1 = sign(p_p, p_a, p_b);
			float d2 = sign(p_p, p_b, p_c);
			float d3 = sign(p_p, p_c, p_a);
			bool hasNeg = (d1 < -spk::Constants::angularPrecision) || (d2 < -spk::Constants::angularPrecision) || (d3 < -spk::Constants::angularPrecision);
			bool hasPos = (d1 > spk::Constants::angularPrecision) || (d2 > spk::Constants::angularPrecision) || (d3 > spk::Constants::angularPrecision);
			return (hasNeg && hasPos) == false;
		};

		while (indices.size() > 2)
		{
			bool earFound = false;
			for (size_t i = 0; i < indices.size(); ++i)
			{
				size_t prev = indices[(i + indices.size() - 1) % indices.size()];
				size_t curr = indices[i];
				size_t next = indices[(i + 1) % indices.size()];

				if (cross2D(coords[prev], coords[curr], coords[next]) <= spk::Constants::angularPrecision)
				{
					continue;
				}

				bool hasPoint = false;
				for (size_t j = 0; j < indices.size(); ++j)
				{
					size_t idx = indices[j];
					if (idx == prev || idx == curr || idx == next)
					{
						continue;
					}
					if (pointInTriangle(coords[idx], coords[prev], coords[curr], coords[next]) == true)
					{
						hasPoint = true;
						break;
					}
				}
				if (hasPoint == true)
				{
					continue;
				}

				triangles.push_back(Polygon::fromLoop({pts[prev], pts[curr], pts[next]}));
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

	Polygon Polygon::makeTriangle(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c)
	{
		Polygon result;

		result._points = {p_a, p_b, p_c};
		result._addEdge(p_a, p_b);
		result._addEdge(p_b, p_c);
		result._addEdge(p_c, p_a);

		return result;
	}

	Polygon Polygon::makeSquare(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d)
	{
		Polygon result;

		result._points = {p_a, p_b, p_c, p_d};
		result._addEdge(p_a, p_b);
		result._addEdge(p_b, p_c);
		result._addEdge(p_c, p_d);
		result._addEdge(p_d, p_a);

		return result;
	}

	std::ostream &operator<<(std::ostream &p_os, const Polygon &p_poly)
	{
		p_os << "{";
		for (size_t i = 0; i < p_poly._edges.size(); i++)
		{
			p_os << p_poly._edges[i];
			if (i + 1 < p_poly._edges.size())
			{
				p_os << ", ";
			}
		}
		p_os << "} concave: " << (p_poly.isConvex() == false ? "true" : "false");
		return p_os;
	}

	std::wostream &operator<<(std::wostream &p_wos, const Polygon &p_poly)
	{
		p_wos << L"{";
		for (size_t i = 0; i < p_poly._edges.size(); i++)
		{
			p_wos << p_poly._edges[i];
			if (i + 1 < p_poly._edges.size())
			{
				p_wos << L", ";
			}
		}
		p_wos << L"} concave: " << (p_poly.isConvex() == false ? L"true" : L"false");
		return p_wos;
	}
}
