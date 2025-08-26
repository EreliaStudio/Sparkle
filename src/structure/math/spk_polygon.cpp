#include "structure/math/spk_polygon.hpp"
#include "spk_debug_macro.hpp"
#include "structure/math/spk_constants.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <utility>

namespace
{
	// Clamp a float in [min,max]
	float clampf(float p_v, float p_lo, float p_hi)
	{
		return std::max(p_lo, std::min(p_v, p_hi));
	}

	// Gather split params along 'base' when 'other' is colinear and overlapping
	void collectSplitTsForColinearOverlap(const spk::Edge &p_base, const spk::Edge &p_other, std::vector<float> &p_ts)
	{
		if (p_base.isColinear(p_other) == false)
		{
			return;
		}

		const float len2 = p_base.delta().dot(p_base.delta());
		if (FLOAT_EQ(len2, 0.0f) == true)
		{
			return;
		}

		float t0 = p_base.project(p_other.first());
		float t1 = p_base.project(p_other.second());
		if (t1 < t0)
		{
			std::swap(t0, t1);
		}

		float lo = clampf(t0, 0.0f, len2);
		float hi = clampf(t1, 0.0f, len2);

		if (FLOAT_NEQ(hi - lo, 0.0f) == true)
		{
			p_ts.push_back(lo);
			p_ts.push_back(hi);
		}
	}

	void collectSplitTsForIntersection(const spk::Edge &p_base, const spk::Edge &p_other, const spk::Vector3 &p_normal, std::vector<float> &p_ts)
	{
		if (p_base.isColinear(p_other) == true)
		{
			return;
		}

		spk::Vector3 r = p_base.delta();
		spk::Vector3 s = p_other.delta();
		spk::Vector3 qp = p_other.first() - p_base.first();

		float denom = r.cross(s).dot(p_normal);
		if (FLOAT_EQ(denom, 0.0f) == true)
		{
			return;
		}

		float t = qp.cross(s).dot(p_normal) / denom;
		float u = qp.cross(r).dot(p_normal) / denom;

		const float eps = 1e-6f;
		if (t > eps && t < 1.0f - eps && u >= -eps && u <= 1.0f + eps)
		{
			spk::Vector3 inter = p_base.first() + r * t;
			p_ts.push_back(p_base.project(inter));
		}
	}

	// Split one edge by collected ts
	void splitEdgeByTs(const spk::Edge &p_e, const std::vector<float> &p_ts, std::vector<spk::Edge> &p_out)
	{
		const float len = p_e.delta().norm();
		const float len2 = len * len;
		if (FLOAT_EQ(len, 0.0f) == true)
		{
			return;
		}

		std::vector<float> cuts;
		cuts.reserve(p_ts.size() + 2);
		cuts.push_back(0.0f);
		for (float t : p_ts)
		{
			if (t > 0.0f && t < len2)
			{
				cuts.push_back(t);
			}
		}
		cuts.push_back(len2);
		std::sort(cuts.begin(), cuts.end());
		cuts.erase(std::unique(cuts.begin(), cuts.end(), [](float p_a, float p_b) { return FLOAT_EQ(p_a, p_b) == true; }), cuts.end());

		for (size_t i = 1; i < cuts.size(); ++i)
		{
			float t0 = cuts[i - 1], t1 = cuts[i];
			if (FLOAT_EQ(t1 - t0, 0.0f) == true)
			{
				continue;
			}

			const spk::Vector3 a = p_e.first() + p_e.direction() * (t0 / len);
			const spk::Vector3 b = p_e.first() + p_e.direction() * (t1 / len);
			p_out.emplace_back(a, b);
		}
	}

	void splitAllEdges(std::vector<spk::Edge> &p_base, const std::vector<spk::Edge> &p_other, const spk::Vector3 &p_normal)
	{
		std::vector<spk::Edge> result;
		result.reserve(p_base.size() * 2);
		for (const auto &e : p_base)
		{
			std::vector<float> ts;
			for (const auto &o : p_other)
			{
				collectSplitTsForColinearOverlap(e, o, ts);
				collectSplitTsForIntersection(e, o, p_normal, ts);
			}
			splitEdgeByTs(e, ts, result);
		}
		p_base.swap(result);
	}

	// Remove sub-edges that appear twice with opposite orientation.
	// If edges overlap with the same orientation, keep a single copy.
	std::vector<spk::Edge> subtractInternalShared(const std::vector<spk::Edge> &p_a, const std::vector<spk::Edge> &p_b)
	{
		std::vector<bool> usedB(p_b.size(), false);
		std::vector<spk::Edge> out;
		out.reserve(p_a.size() + p_b.size());

		for (size_t i = 0; i < p_a.size(); ++i)
		{
			bool consumed = false;
			for (size_t j = 0; j < p_b.size(); ++j)
			{
				if (usedB[j] == true)
				{
					continue;
				}
				if (p_a[i].isInverse(p_b[j]) == true)
				{
					usedB[j] = true;
					consumed = true;
					break;
				}
				if (p_a[i] == p_b[j])
				{
					usedB[j] = true;
					out.push_back(p_a[i]);
					consumed = true;
					break;
				}
			}
			if (consumed == false)
			{
				out.push_back(p_a[i]);
			}
		}
		for (size_t j = 0; j < p_b.size(); ++j)
		{
			if (usedB[j] == false)
			{
				out.push_back(p_b[j]);
			}
		}
		return out;
	}

	std::vector<spk::Vector3> stitchLoop(const std::vector<spk::Edge> &p_edges, const spk::Vector3 &p_normal)
	{
		if (p_edges.empty() == true)
		{
			return {};
		}

		spk::Vector3 up = p_normal.normalize();
		spk::Vector3 yAxis = spk::Vector3(0, 1, 0);
		if (up == yAxis || up == yAxis.inverse())
		{
			yAxis = spk::Vector3(0, 0, 1);
		}
		spk::Vector3 xAxis = yAxis.cross(up).normalize();
		yAxis = up.cross(xAxis);

		std::map<spk::Vector3, spk::Vector2> coords;
		std::map<spk::Vector3, std::vector<std::pair<size_t, bool>>> adj;
		for (size_t i = 0; i < p_edges.size(); ++i)
		{
			const spk::Edge &e = p_edges[i];
			auto insert = [&](const spk::Vector3 &p_v)
			{
				if (coords.find(p_v) == coords.end())
				{
					coords[p_v] = spk::Vector2(p_v.dot(xAxis), p_v.dot(yAxis));
				}
			};
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

namespace spk
{
	void Polygon::_addEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b)
	{
		_edges.push_back(spk::Edge(p_a, p_b));
		_points.push_back(p_a);
	}

	void Polygon::_updateEdges() const
	{
		if (_edgesDirty == true)
		{
			_edges.clear();
			if (_points.size() >= 2)
			{
				for (size_t i = 1; i < _points.size(); ++i)
				{
					_edges.emplace_back(_points[i - 1], _points[i]);
				}
				_edges.emplace_back(_points.back(), _points.front());
			}
			_edgesDirty = false;
		}
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
					return false;
				}
			}
			else
			{
				if (val >= -p_eps)
				{
					return false;
				}
			}
		}

		return true;
	}

	const std::vector<spk::Edge> &Polygon::edges() const
	{
		_updateEdges();
		return _edges;
	}

	const std::vector<spk::Vector3> &Polygon::points() const
	{
		return _points;
	}

	std::vector<spk::Vector3> &Polygon::points()
	{
		_edgesDirty = true;
		return _points;
	}

	bool Polygon::isPlanar() const
	{
		const auto &edgesRef = edges();
		if (edgesRef.size() < 2)
		{
			return false;
		}

		spk::Vector3 expectedNormal = normal();

		for (size_t i = 2; i < edgesRef.size(); i++)
		{
			float dot = expectedNormal.dot(edgesRef[i].direction());

			if (dot != 0)
			{
				return false;
			}
		}
		return true;
	}

	spk::Vector3 Polygon::normal() const
	{
		const auto &edgesRef = edges();
		if (edgesRef.size() < 2)
		{
			GENERATE_ERROR("Can't generate a normal for a polygon with less than 2 edges");
		}

		return (edgesRef[0].direction().cross(edgesRef[1].direction()));
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

		spk::Vector3 n1 = normal().normalize();
		spk::Vector3 n2 = p_other.normal().normalize();

		if (((n1 == n2) == false) && ((n1 == n2.inverse()) == false))
		{
			return false;
		}

		float d1 = n1.dot(edges()[0].first());
		float d2 = n1.dot(p_other.edges()[0].first());

		return (FLOAT_EQ(d1, d2) == true);
	}

	bool Polygon::isAdjacent(const Polygon &p_other) const
	{
		if (isCoplanar(p_other) == false)
		{
			return false;
		}

		const auto &edgesRef = edges();
		for (const auto &edgeA : edgesRef)
		{
			for (const auto &edgeB : p_other.edges())
			{
				if (edgeA.isColinear(edgeB) == true)
				{
					if (edgeA.contains(edgeB.first(), false) == true || edgeA.contains(edgeB.second(), false) == true ||
						edgeB.contains(edgeA.first(), false) == true || edgeB.contains(edgeA.second(), false) == true)
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
		const auto &edgesRef = edges();
		if (edgesRef.size() < 3)
		{
			return false;
		}

		spk::Vector3 polyNormal = normal();
		float orientation = 0;

		for (size_t i = 0; i < edgesRef.size(); i++)
		{
			const spk::Edge &current = edgesRef[i];
			const spk::Edge &next = edgesRef[(i + 1) % edgesRef.size()];

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

		const auto &edgesRef = edges();
		for (const auto &edgeA : edgesRef)
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

		if (_isPointInside(p_other, edgesRef[0].first(), eps) == true)
		{
			return true;
		}

		return false;
	}

	bool Polygon::contains(const spk::Vector3 &p_point) const
	{
		const float eps = 1e-6f;
		spk::Vector3 n = normal();
		const auto &edgesRef = edges();
		float orient = edgesRef[0].direction().cross(edgesRef[1].direction()).dot(n);

		for (const auto &edge : edgesRef)
		{
			float val = edge.direction().cross(p_point - edge.first()).dot(n);

			if (std::fabs(val) <= eps && edge.contains(p_point, false) == true)
			{
				return true;
			}

			if (orient > 0)
			{
				if (val < -eps)
				{
					return false;
				}
			}
			else
			{
				if (val > eps)
				{
					return false;
				}
			}
		}

		return true;
	}

	bool Polygon::contains(const Polygon &p_polygon) const
	{
		if (isCoplanar(p_polygon) == false)
		{
			return false;
		}
		for (const auto &pt : p_polygon.points())
		{
			if (contains(pt) == false)
			{
				return false;
			}
		}
		return true;
	}

	Polygon Polygon::fuze(const Polygon &p_other) const
	{
		if (isCoplanar(p_other) == false)
		{
			GENERATE_ERROR("Polygons must be coplanar");
		}

		if (isAdjacent(p_other) == false && isOverlapping(p_other) == false)
		{
			return Polygon();
		}

		spk::Vector3 n = normal();

		std::vector<spk::Edge> A = edges();
		std::vector<spk::Edge> B = p_other.edges();

		splitAllEdges(A, B, n);
		splitAllEdges(B, A, n);

		std::vector<spk::Edge> boundary = subtractInternalShared(A, B);
		if (boundary.empty())
		{
			GENERATE_ERROR("Fused polygon is empty or degenerate");
		}

		std::vector<spk::Vector3> loop = stitchLoop(boundary, n);
		if (loop.size() < 4)
		{
			GENERATE_ERROR("Fused polygon could not be stitched");
		}

		return Polygon::fromLoop(loop);
	}

	Polygon Polygon::fromLoop(const std::vector<spk::Vector3> &p_vs)
	{
		Polygon r;
		if (p_vs.size() < 2)
		{
			return r;
		}
		for (size_t i = 1; i < p_vs.size(); ++i)
		{
			r._addEdge(p_vs[i - 1], p_vs[i]);
		}
		r._addEdge(p_vs.back(), p_vs.front());
		return r;
	}

	Polygon Polygon::makeTri(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c)
	{
		Polygon result;

		result._addEdge(p_a, p_b);
		result._addEdge(p_b, p_c);
		result._addEdge(p_c, p_a);

		return result;
	}

	Polygon Polygon::makeSquare(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d)
	{
		Polygon result;

		result._addEdge(p_a, p_b);
		result._addEdge(p_b, p_c);
		result._addEdge(p_c, p_d);
		result._addEdge(p_d, p_a);

		return result;
	}

	std::ostream &operator<<(std::ostream &p_os, const Polygon &p_poly)
	{
		const auto &edgesRef = p_poly.edges();
		p_os << "{";
		for (size_t i = 0; i < edgesRef.size(); i++)
		{
			p_os << edgesRef[i];
			if (i + 1 < edgesRef.size())
			{
				p_os << ", ";
			}
		}
		p_os << "}";
		return p_os;
	}

	std::wostream &operator<<(std::wostream &p_wos, const Polygon &p_poly)
	{
		const auto &edgesRef = p_poly.edges();
		p_wos << L"{";
		for (size_t i = 0; i < edgesRef.size(); i++)
		{
			p_wos << edgesRef[i];
			if (i + 1 < edgesRef.size())
			{
				p_wos << L", ";
			}
		}
		p_wos << L"}";
		return p_wos;
	}
}
