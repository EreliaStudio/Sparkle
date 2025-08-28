#include "structure/math/spk_polygon.hpp"
#include "structure/math/spk_constants.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace spk
{
	void Polygon::_addEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b)
	{
		_edges.push_back(spk::Edge(p_a, p_b));
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

		spk::Vector3 expectedNormal = normal();

		for (size_t i = 2; i < _edges.size(); i++)
		{
			float dot = expectedNormal.dot(_edges[i].direction());

			if (dot != 0)
			{
				return false;
			}
		}
		return true;
	}

	spk::Vector3 Polygon::normal() const
	{
		if (_edges.size() < 2)
		{
			GENERATE_ERROR("Can't generate a normal for a polygon with less than 2 edges");
		}

		return (_edges[0].direction().cross(_edges[1].direction()));
	}

	spk::Plane Polygon::plane() const
	{
		if (_points.empty() == true)
		{
			GENERATE_ERROR("Can't generate a plane for a polygon without points");
		}

		return (spk::Plane(normal(), _points[0]));
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

		spk::Vector3 currentNormal = normal();
		spk::Vector3 otherNormal = p_other.normal();

		if ((currentNormal == otherNormal || currentNormal.inverse() == otherNormal) == false)
		{
			return false;
		}

		float currentOffset = currentNormal.dot(_edges[0].first());
		float otherOffset = currentNormal.dot(p_other.edges()[0].first());

		return (FLOAT_EQ(currentOffset, otherOffset) == true);
	}

	bool Polygon::isAdjacent(const Polygon &p_other) const
	{
		if (isCoplanar(p_other) == false)
		{
			return false;
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

	bool Polygon::isConvex(float p_eps, bool p_strictly) const
	{
		if (_edges.size() < 3)
		{
			return false;
		}

		spk::Vector3 polyNormal = normal();
		float orientation = 0;

		for (size_t i = 0; i < _edges.size(); i++)
		{
			const spk::Edge &current = _edges[i];
			const spk::Edge &next = _edges[(i + 1) % _edges.size()];

			float dot = current.direction().cross(next.direction()).dot(polyNormal);

			if (std::fabs(dot) <= p_eps)
			{
				if (p_strictly == true)
				{
					return false;
				}
				continue;
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

		const float eps = 1e-6f;
		spk::Vector3 polyNormal = normal();

		for (const auto &edgeA : _edges)
		{
			for (const auto &edgeB : p_other.edges())
			{
				if (_edgesIntersect(edgeA, edgeB, polyNormal, eps) == true)
				{
					return true;
				}
			}
		}

		if (_isPointInside(*this, p_other.edges()[0].first(), eps) == true)
		{
			return true;
		}

		if (_isPointInside(p_other, _edges[0].first(), eps) == true)
		{
			return true;
		}

		return false;
	}

	bool Polygon::contains(const spk::Vector3 &p_point) const
	{
		const float eps = 1e-6f;
		return _isPointInside(*this, p_point, eps);
	}

	bool Polygon::contains(const Polygon &p_polygon) const
	{
		if (isCoplanar(p_polygon) == false)
		{
			return false;
		}

		for (const auto &edge : p_polygon.edges())
		{
			if (_isPointInside(*this, edge.first(), 1e-6f) == false || _isPointInside(*this, edge.second(), 1e-6f) == false)
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
