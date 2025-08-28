#include "structure/math/spk_constants.hpp"
#include "structure/math/spk_polygon.hpp"
#include "structure/math/spk_vector2.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <utility>
#include <vector>

namespace
{
	void collectSplitTsForColinearOverlap(const spk::Edge &p_e, const spk::Edge &p_o, std::vector<float> &p_ts)
	{
		if (p_e.isColinear(p_o) == false)
		{
			return;
		}

		const float len = p_e.delta().norm();
		const float dirDot = p_e.direction().dot(p_o.direction());
		if (std::fabs(std::fabs(dirDot) - 1.0f) > spk::Constants::pointPrecision)
		{
			return;
		}

		const spk::Vector3 diff = p_o.first() - p_e.first();
		const float t1 = diff.dot(p_e.direction());
		const float t2 = t1 + p_o.delta().norm() * dirDot;

		const float lo = std::min(t1, t2);
		const float hi = std::max(t1, t2);

		if (hi <= spk::Constants::pointPrecision || lo >= (len - spk::Constants::pointPrecision))
		{
			return;
		}

		const float start = std::max(0.0f, lo);
		const float end = std::min(len, hi);

		const float tol = spk::Constants::pointPrecision;
		if (start > tol && start < len - tol)
		{
			p_ts.push_back(start / len);
		}
		if (end > tol && end < len - tol)
		{
			p_ts.push_back(end / len);
		}
	}

	void collectSplitTsForIntersection(const spk::Edge &p_e, const spk::Edge &p_o, const spk::Vector3 &p_normal, std::vector<float> &p_ts)
	{
		const float o1 = p_o.orientation(p_e.first(), p_normal);
		const float o2 = p_o.orientation(p_e.second(), p_normal);
		const float eps = spk::Constants::pointPrecision;
		if ((o1 > eps && o2 > eps) || (o1 < -eps && o2 < -eps))
		{
			return;
		}

		const float o3 = p_e.orientation(p_o.first(), p_normal);
		const float o4 = p_e.orientation(p_o.second(), p_normal);
		if ((o3 > eps && o4 > eps) || (o3 < -eps && o4 < -eps))
		{
			return;
		}

		const spk::Vector3 r = p_e.delta();
		const spk::Vector3 s = p_o.delta();

		const spk::Vector3 qp = p_o.first() - p_e.first();
		const float rxs = r.cross(s).dot(p_normal.normalize());
		if (std::fabs(rxs) <= spk::Constants::pointPrecision)
		{
			return;
		}

		const float t = qp.cross(s).dot(p_normal.normalize()) / rxs;
		const float len = r.norm();

		const float pos = t * len;
		if (pos > spk::Constants::pointPrecision && pos < len - spk::Constants::pointPrecision)
		{
			p_ts.push_back(pos / len);
		}
	}

	void splitEdgeByTs(const spk::Edge &p_e, const std::vector<float> &p_ts, std::vector<spk::Edge> &p_out)
	{
		std::vector<float> ts = p_ts;
		ts.push_back(0.0f);
		ts.push_back(1.0f);
		std::sort(ts.begin(), ts.end());
		for (size_t i = 1; i < ts.size(); ++i)
		{
			float a = ts[i - 1];
			float b = ts[i];
			if (b - a <= spk::Constants::pointPrecision)
			{
				continue;
			}
			spk::Vector3 start = p_e.first() + p_e.delta() * a;
			spk::Vector3 end = p_e.first() + p_e.delta() * b;
			p_out.emplace_back(start, end);
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
		std::vector<spk::Edge> edges;
		edges.reserve(p_edges.size());

		auto getKey = [&](const spk::Vector3 &p_v) -> spk::Vector3
		{
			for (const auto &kv : coords)
			{
				if (kv.first == p_v)
				{
					return kv.first;
				}
			}
			coords[p_v] = spk::Vector2(p_v.dot(xAxis), p_v.dot(yAxis));
			return p_v;
		};

		for (size_t i = 0; i < p_edges.size(); ++i)
		{
			const spk::Edge &e = p_edges[i];
			spk::Vector3 a = getKey(e.first());
			spk::Vector3 b = getKey(e.second());
			edges.emplace_back(a, b);
			adj[a].push_back({i, true});
			adj[b].push_back({i, false});
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

		std::vector<bool> used(edges.size(), false);
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
				spk::Vector3 next3 = forward == true ? edges[idx].second() : edges[idx].first();
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
			spk::Vector3 next3 = bestForward == true ? edges[bestIdx].second() : edges[bestIdx].first();
			loop.push_back(next3);
			curDir = (coords[next3] - coords[cur3]).normalize();
			cur3 = next3;
			if (cur3 == start)
			{
				break;
			}
		}
		loop.erase(std::unique(loop.begin(), loop.end(), [](const spk::Vector3 &p_a, const spk::Vector3 &p_b) { return p_a == p_b; }), loop.end());
		if (loop.size() >= 3 && ((loop.front() == loop.back()) == false))
		{
			loop.push_back(loop.front());
		}
		return loop;
	}
}

namespace spk
{
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

		std::vector<spk::Edge> A = _edges;
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
			GENERATE_ERROR("Fused polygon could not be stitched (Loop of " + std::to_string(loop.size()) + " element(s))");
		}

		return spk::Polygon::fromLoop(loop);
	}
}
