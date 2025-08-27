#include "structure/math/spk_polygon.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <utility>
#include <variant>
#include <vector>

namespace spk
{
	const std::vector<spk::Edge> &Polygon::edges() const
	{
		return (_edges);
	}

	std::vector<spk::Vector3> Polygon::points() const
	{
		std::vector<spk::Vector3> pts;
		pts.reserve(_edges.size());
		for (const auto &edge : _edges)
		{
			pts.push_back(edge.first());
		}
		return pts;
	}

	void Polygon::_addEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b)
	{
		_edges.push_back(spk::Edge(p_a, p_b));
	}

	bool Polygon::_edgesIntersect(const spk::Edge &p_a, const spk::Edge &p_b, const spk::Vector3 &p_normal, float p_eps)
	{
		float o1 = p_a.orientation(p_b.first(), p_normal);
		float o2 = p_a.orientation(p_b.second(), p_normal);
		float o3 = p_b.orientation(p_a.first(), p_normal);
		float o4 = p_b.orientation(p_a.second(), p_normal);
		bool cond1 = ((o1 > p_eps && o2 < -p_eps) || (o1 < -p_eps && o2 > p_eps));
		bool cond2 = ((o3 > p_eps && o4 < -p_eps) || (o3 < -p_eps && o4 > p_eps));
		return (cond1 == true && cond2 == true);
	}

	bool Polygon::_isPointInside(const Polygon &p_poly, const spk::Vector3 &p_point, float p_eps)
	{
		const auto &edges = p_poly.edges();
		spk::Vector3 n = p_poly.normal();
		float orient = edges[0].direction().cross(edges[1].direction()).dot(n);
		for (size_t i = 0; i < edges.size(); i++)
		{
			const spk::Edge &edge = edges[i];
			float val = edge.direction().cross(p_point - edge.first()).dot(n);
			if (orient > 0)
			{
				if (val <= p_eps)
				{
					return (false);
				}
			}
			else
			{
				if (val >= -p_eps)
				{
					return (false);
				}
			}
		}
		return (true);
	}

	bool Polygon::isPlanar() const
	{
		if (_edges.size() < 2)
		{
			return (false);
		}
		spk::Vector3 expectedNormal = normal();
		for (size_t i = 2; i < _edges.size(); i++)
		{
			float dot = expectedNormal.dot(_edges[i].direction());
			if (FLOAT_NEQ(dot, 0.0f) == true)
			{
				return (false);
			}
		}
		return (true);
	}

	spk::Vector3 Polygon::normal() const
	{
		if (_edges.size() < 2)
		{
			GENERATE_ERROR("Can't generate a normal for a polygon with less than 2 edges");
		}
		return (_edges[0].direction().cross(_edges[1].direction()));
	}

	bool Polygon::isCoplanar(const Polygon &p_other) const
	{
		if (isPlanar() == false)
		{
			return (false);
		}
		if (p_other.isPlanar() == false)
		{
			return (false);
		}
		spk::Vector3 currentNormal = normal();
		spk::Vector3 otherNormal = p_other.normal();
		if ((currentNormal == otherNormal || currentNormal.inverse() == otherNormal) == false)
		{
			return (false);
		}
		float currentOffset = currentNormal.dot(_edges[0].first());
		float otherOffset = currentNormal.dot(p_other.edges()[0].first());
		return (FLOAT_EQ(currentOffset, otherOffset) == true);
	}

	bool Polygon::isAdjacent(const Polygon &p_other) const
	{
		if (isCoplanar(p_other) == false)
		{
			return (false);
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
						return (true);
					}
				}
			}
		}
		return (false);
	}

	bool Polygon::isConvex(float p_eps, bool p_strictly) const
	{
		if (_edges.size() < 3)
		{
			return (false);
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
					return (false);
				}
				continue;
			}
			if (orientation == 0)
			{
				orientation = (dot > 0) ? 1.0f : -1.0f;
			}
			else if (((dot > 0) ? 1.0f : -1.0f) != orientation)
			{
				return (false);
			}
		}
		return (orientation != 0);
	}

	bool Polygon::isOverlapping(const Polygon &p_other) const
	{
		if (isCoplanar(p_other) == false)
		{
			return (false);
		}
		const float eps = 1e-6f;
		spk::Vector3 polyNormal = normal();
		for (const auto &edgeA : _edges)
		{
			for (const auto &edgeB : p_other.edges())
			{
				if (_edgesIntersect(edgeA, edgeB, polyNormal, eps) == true)
				{
					return (true);
				}
			}
		}
		if (_isPointInside(*this, p_other.edges()[0].first(), eps) == true)
		{
			return (true);
		}
		if (_isPointInside(p_other, _edges[0].first(), eps) == true)
		{
			return (true);
		}
		return (false);
	}

	bool Polygon::contains(const spk::Vector3 &p_point) const
	{
		const float eps = spk::Constants::pointPrecision;
		spk::Vector3 n = normal();
		float dist = n.dot(p_point - _edges[0].first());
		if (std::fabs(dist) > eps)
		{
			return (false);
		}
		for (const auto &edge : _edges)
		{
			if (edge.contains(p_point) == true)
			{
				return (true);
			}
		}
		return _isPointInside(*this, p_point, eps);
	}

	bool Polygon::contains(const Polygon &p_polygon) const
	{
		if (isCoplanar(p_polygon) == false)
		{
			return (false);
		}
		for (const auto &edge : p_polygon.edges())
		{
			if (contains(edge.first()) == false)
			{
				return (false);
			}
		}
		return (true);
	}

	// Helper functions for fuze
	namespace
	{
		void splitEdgeByTs(const spk::Edge &p_e, const std::vector<float> &p_ts, std::vector<spk::Edge> &p_out)
		{
			std::vector<float> sorted = p_ts;
			sorted.erase(
				std::remove_if(
					sorted.begin(),
					sorted.end(),
					[](float v) { return v <= spk::Constants::pointPrecision || v >= 1.0f - spk::Constants::pointPrecision; }),
				sorted.end());
			std::sort(sorted.begin(), sorted.end());
			sorted.erase(std::unique(sorted.begin(), sorted.end(), [](float a, float b) { return FLOAT_EQ(a, b); }), sorted.end());
			spk::Vector3 A = p_e.first();
			for (float t : sorted)
			{
				spk::Vector3 B = p_e.first() + p_e.delta() * t;
				if ((A == B) == true)
				{
					continue;
				}
				p_out.emplace_back(A, B);
				A = B;
			}
			if ((A == p_e.second()) == false)
			{
				p_out.emplace_back(A, p_e.second());
			}
		}

		void splitAllEdges(std::vector<spk::Edge> &p_base, const std::vector<spk::Edge> &p_other, const spk::Vector3 &p_normal)
		{
			std::vector<spk::Edge> result;
			for (const auto &e : p_base)
			{
				std::vector<float> ts;
				for (const auto &o : p_other)
				{
					if (e.isColinear(o) == true)
					{
						continue;
					}
					float denom = e.direction().cross(o.direction()).dot(p_normal);
					if (std::fabs(denom) <= 1e-6f)
					{
						continue;
					}
					float t = (o.first() - e.first()).cross(o.delta()).dot(p_normal) / denom;
					float u = (o.first() - e.first()).cross(e.delta()).dot(p_normal) / denom;
					if (t > 0.0f && t < 1.0f && u > 0.0f && u < 1.0f)
					{
						ts.push_back(t);
					}
				}
				splitEdgeByTs(e, ts, result);
			}
			p_base = result;
		}

		std::vector<spk::Edge> subtractInternalShared(const std::vector<spk::Edge> &p_a, const std::vector<spk::Edge> &p_b)
		{
			std::vector<spk::Edge> out;
			for (const auto &e : p_a)
			{
				bool keep = true;
				for (const auto &f : p_b)
				{
					if (e.isSame(f) == true && e.isInverse(f) == true)
					{
						keep = false;
						break;
					}
				}
				if (keep == true)
				{
					out.push_back(e);
				}
			}
			return out;
		}

		std::vector<spk::Vector3> stitchLoop(const std::vector<spk::Edge> &p_edges, const spk::Vector3 &p_normal)
		{
			std::map<spk::Vector3, std::vector<std::pair<size_t, bool>>> adj;
			std::map<spk::Vector3, spk::Vector2> coords;
			spk::Vector3 xAxis = p_edges[0].delta().normalize();
			spk::Vector3 yAxis = p_normal.cross(xAxis);
			auto insert = [&](const spk::Vector3 &p_v)
			{
				if (coords.find(p_v) == coords.end())
				{
					coords[p_v] = spk::Vector2(p_v.dot(xAxis), p_v.dot(yAxis));
				}
			};
			for (size_t i = 0; i < p_edges.size(); ++i)
			{
				const spk::Edge &e = p_edges[i];
				insert(e.first());
				insert(e.second());
				adj[e.first()].push_back({i, true});
				adj[e.second()].push_back({i, false});
			}
			spk::Vector3 start = adj.begin()->first;
			spk::Vector2 startCoord = coords[start];
			for (const auto &kv : coords)
			{
				const spk::Vector2 &c = kv.second;
				if (c.y < startCoord.y || (FLOAT_EQ(c.y, startCoord.y) == true && c.x < startCoord.x))
				{
					start = kv.first;
					startCoord = c;
				}
			}
			std::vector<bool> used(p_edges.size(), false);
			std::vector<spk::Vector3> loop;
			loop.push_back(start);
			spk::Vector3 cur3 = start;
			spk::Vector2 curDir(1.0f, 0.0f);
			while (true)
			{
				float bestAng = -std::numeric_limits<float>::infinity();
				size_t bestIdx = SIZE_MAX;
				bool bestForward = true;
				for (auto &opt : adj[cur3])
				{
					size_t idx = opt.first;
					bool forward = opt.second;
					if (used[idx] == true)
					{
						continue;
					}
					spk::Vector3 next3 = forward == true ? p_edges[idx].second() : p_edges[idx].first();
					spk::Vector2 dir = (coords[next3] - coords[cur3]).normalize();
					float cross = curDir.x * dir.y - curDir.y * dir.x;
					float dot = curDir.x * dir.x + curDir.y * dir.y;
					float ang = std::atan2(cross, dot);
					if (ang > bestAng)
					{
						bestAng = ang;
						bestIdx = idx;
						bestForward = forward;
					}
				}
				if (bestIdx == SIZE_MAX)
				{
					break;
				}
				used[bestIdx] = true;
				spk::Vector3 next3 = bestForward == true ? p_edges[bestIdx].second() : p_edges[bestIdx].first();
				loop.push_back(next3);
				curDir = (coords[next3] - coords[cur3]).normalize();
				cur3 = next3;
				if (cur3 == start)
				{
					break;
				}
			}
			if (loop.size() >= 3 && ((loop.front() == loop.back()) == false))
			{
				loop.push_back(loop.front());
			}
			return loop;
		}
	}

	Polygon Polygon::fuze(const Polygon &p_other, bool p_needVerification) const
	{
		if (isCoplanar(p_other) == false)
		{
			GENERATE_ERROR("Polygons must be coplanar");
		}
		if (p_needVerification == true && isAdjacent(p_other) == false && isOverlapping(p_other) == false)
		{
			return spk::Polygon();
		}

		spk::cout << " ---- Fuzing polygons ----" << std::endl;
		spk::cout << "A: " << *this << std::endl;
		spk::cout << "B: " << p_other << std::endl;

		spk::Vector3 n = normal();
		std::vector<spk::Edge> A = _edges;
		std::vector<spk::Edge> B = p_other.edges();
		splitAllEdges(A, B, n);
		splitAllEdges(B, A, n);
		std::vector<spk::Edge> boundary = subtractInternalShared(A, B);
		std::vector<spk::Edge> uniqueB = subtractInternalShared(B, A);
		boundary.insert(boundary.end(), uniqueB.begin(), uniqueB.end());
		if (boundary.empty())
		{
			GENERATE_ERROR("Fused polygon is empty or degenerate");
		}
		std::vector<spk::Vector3> loop = stitchLoop(boundary, n);
		spk::cout << "Fused loop has " << loop.size() << " points" << std::endl;
		spk::cout << "Fused loop: " << std::endl;
		for (size_t i = 0; i < loop.size(); i++)
		{
			const spk::Vector3 &pt = loop[i];
			spk::cout << "    [" << i << "] : " << pt << std::endl;
		}
		if (loop.size() < 4)
		{
			GENERATE_ERROR("Fused polygon could not be stitched (Loop size : " + std::to_string(loop.size()) + ")");
		}
		return spk::Polygon::fromLoop(loop);
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
		pts.push_back(pts.front());
		for (size_t i = 1; i < pts.size(); ++i)
		{
			r._addEdge(pts[i - 1], pts[i]);
		}
		return r;
	}

	Polygon Polygon::makeTriangle(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c)
	{
		Polygon result;
		result._addEdge(p_a, p_b);
		result._addEdge(p_b, p_c);
		result._addEdge(p_c, p_a);
		return (result);
	}

	Polygon Polygon::makeSquare(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d)
	{
		Polygon result;
		result._addEdge(p_a, p_b);
		result._addEdge(p_b, p_c);
		result._addEdge(p_c, p_d);
		result._addEdge(p_d, p_a);
		return (result);
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
