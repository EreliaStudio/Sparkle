#include "structure/engine/spk_obj_mesh.hpp"
#include "structure/math/spk_constants.hpp"
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/math/spk_vector4.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <utility>
#include <vector>

namespace tmp
{
	class Edge
	{
	private:
		spk::Vector3 _first;
		spk::Vector3 _second;

		spk::Vector3 _delta;
		spk::Vector3 _direction;

	public:
		Edge(const spk::Vector3 &p_first, const spk::Vector3 &p_second) :
			_first(p_first),
			_second(p_second)
		{
			if (_second == _first)
			{
				GENERATE_ERROR("Can't create an edge of lenght == 0");
			}

			_delta = (_second - _first);
			_direction = _delta.normalize();
		}

		const spk::Vector3 &first() const
		{
			return (_first);
		}

		const spk::Vector3 &second() const
		{
			return (_second);
		}

		const spk::Vector3 &delta() const
		{
			return (_delta);
		}

		const spk::Vector3 &direction() const
		{
			return (_direction);
		}

		float orientation(const spk::Vector3 &p_point, const spk::Vector3 &p_normal) const
		{
			return ((_second - _first).cross(p_point - _first)).dot(p_normal);
		}

		bool contains(const spk::Vector3 &p_point, bool p_checkAlignment = true) const
		{
			if (p_point == _first)
			{
				return (true);
			}
			const spk::Vector3 v = p_point - _first;

			if (p_checkAlignment == true && v.normalize() != _direction)
			{
				return (false);
			}

			const float t = v.dot(_direction);

			const float len = (_second - _first).dot(_direction);

			return (t >= 0) && (t <= len);
		}

		float project(const spk::Vector3 &p_point) const
		{
			return (_delta.dot(p_point - _first));
		}

		bool isInverse(const Edge &p_other) const
		{
			return (_first == p_other.second() && _second == p_other.first());
		}

		Edge inverse() const
		{
			return (Edge(_second, _first));
		}

		bool isParallel(const Edge &p_other) const
		{
			return (direction() == p_other.direction() || direction() == p_other.direction().inverse());
		}

		bool isColinear(const Edge &p_other) const
		{
			if (isParallel(p_other) == false)
			{
				return (false);
			}

			spk::Vector3 delta = (p_other._first - _first);

			if (delta == spk::Vector3(0, 0, 0))
			{
				return (true);
			}

			return (std::fabs(delta.normalize().dot(_direction)) == 1);
		}

		bool operator==(const Edge &p_other) const
		{
			return (first() == p_other.first()) && (second() == p_other.second());
		}

		bool isSame(const Edge &p_other) const
		{
			return ((first() == p_other.first()) && (second() == p_other.second()) || (first() == p_other.second()) && (second() == p_other.first()));
		}

		bool operator<(const Edge &p_other) const
		{
			if (first() != p_other.first())
			{
				return (first() < p_other.first());
			}
			return (second() < p_other.second());
		}

		friend std::ostream &operator<<(std::ostream &p_os, const Edge &p_edge)
		{
			p_os << "(" << p_edge.first() << " -> " << p_edge.second() << ")";
			return p_os;
		}

		friend std::wostream &operator<<(std::wostream &p_wos, const Edge &p_edge)
		{
			p_wos << L"(" << p_edge.first() << L" -> " << p_edge.second() << L")";
			return p_wos;
		}
	};

	class Polygon
	{
	private:
		std::vector<tmp::Edge> _edges;

		void _addEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b)
		{
			_edges.push_back(tmp::Edge(p_a, p_b));
		}

		static bool _edgesIntersect(const tmp::Edge &p_a, const tmp::Edge &p_b, const spk::Vector3 &p_normal, float p_eps)
		{
			float o1 = p_a.orientation(p_b.first(), p_normal);
			float o2 = p_a.orientation(p_b.second(), p_normal);
			float o3 = p_b.orientation(p_a.first(), p_normal);
			float o4 = p_b.orientation(p_a.second(), p_normal);

			bool cond1 = ((o1 > p_eps && o2 < -p_eps) || (o1 < -p_eps && o2 > p_eps));
			bool cond2 = ((o3 > p_eps && o4 < -p_eps) || (o3 < -p_eps && o4 > p_eps));

			return (cond1 == true && cond2 == true);
		}

		static bool _isPointInside(const Polygon &p_poly, const spk::Vector3 &p_point, float p_eps)
		{
			const auto &edges = p_poly.edges();
			spk::Vector3 n = p_poly.normal();
			float orient = edges[0].direction().cross(edges[1].direction()).dot(n);

			for (size_t i = 0; i < edges.size(); i++)
			{
				const tmp::Edge &edge = edges[i];
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

	public:
		const std::vector<tmp::Edge> edges() const
		{
			return (_edges);
		}

		bool isPlanar() const
		{
			if (_edges.size() < 2)
			{
				return (false);
			}

			spk::Vector3 expectedNormal = normal();

			for (size_t i = 2; i < _edges.size(); i++)
			{
				float dot = expectedNormal.dot(_edges[i].direction());

				if (dot != 0)
				{
					return (false);
				}
			}
			return (true);
		}

		spk::Vector3 normal() const
		{
			if (_edges.size() < 2)
			{
				GENERATE_ERROR("Can't generate a normal for a polygon with less than 2 edges");
			}

			return (_edges[0].direction().cross(_edges[1].direction()));
		}

		bool isCoplanar(const Polygon &p_other) const
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

			return ((currentNormal == otherNormal || currentNormal.inverse() == otherNormal) && currentNormal.dot(p_other.edges()[0].first()) == 0);
		}

		bool isAdjacent(const Polygon &p_other) const
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

		bool isConvex(float p_eps = 1e-6f, bool p_strictly = false) const
		{
			if (_edges.size() < 3)
			{
				return (false);
			}

			spk::Vector3 polyNormal = normal();
			float orientation = 0;

			for (size_t i = 0; i < _edges.size(); i++)
			{
				const tmp::Edge &current = _edges[i];
				const tmp::Edge &next = _edges[(i + 1) % _edges.size()];

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

		bool isOverlapping(const Polygon &p_other) const
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

		Polygon fuze(const Polygon &p_other) const;

		static Polygon fromLoop(const std::vector<spk::Vector3> &p_vs)
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

		static Polygon makeTriangle(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c)
		{
			Polygon result;

			result._addEdge(p_a, p_b);
			result._addEdge(p_b, p_c);
			result._addEdge(p_c, p_a);

			return (result);
		}

		static Polygon makeSquare(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d)
		{
			Polygon result;

			result._addEdge(p_a, p_b);
			result._addEdge(p_b, p_c);
			result._addEdge(p_c, p_d);
			result._addEdge(p_d, p_a);

			return (result);
		}

		friend std::ostream &operator<<(std::ostream &p_os, const Polygon &p_poly)
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
			p_os << "}";
			return p_os;
		}

		friend std::wostream &operator<<(std::wostream &p_wos, const Polygon &p_poly)
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
			p_wos << L"}";
			return p_wos;
		}
	};
}

namespace
{
	// Clamp a float in [min,max]
	float clampf(float p_v, float p_lo, float p_hi)
	{
		return std::max(p_lo, std::min(p_v, p_hi));
	}

	// Gather split params along 'base' when 'other' is colinear and overlapping
	void collectSplitTsForColinearOverlap(const tmp::Edge &p_base, const tmp::Edge &p_other, std::vector<float> &p_ts)
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

	void collectSplitTsForIntersection(const tmp::Edge &p_base, const tmp::Edge &p_other, const spk::Vector3 &p_normal, std::vector<float> &p_ts)
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
	void splitEdgeByTs(const tmp::Edge &p_e, const std::vector<float> &p_ts, std::vector<tmp::Edge> &p_out)
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

	void splitAllEdges(std::vector<tmp::Edge> &p_base, const std::vector<tmp::Edge> &p_other, const spk::Vector3 &p_normal)
	{
		std::vector<tmp::Edge> result;
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
	std::vector<tmp::Edge> subtractInternalShared(const std::vector<tmp::Edge> &p_a, const std::vector<tmp::Edge> &p_b)
	{
		std::vector<bool> usedB(p_b.size(), false);
		std::vector<tmp::Edge> out;
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

	std::vector<spk::Vector3> stitchLoop( // NOLINT(readability-function-cognitive-complexity)
                const std::vector<tmp::Edge> &p_edges,
                const spk::Vector3 &p_normal)
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
			const tmp::Edge &e = p_edges[i];
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

tmp::Polygon tmp::Polygon::fuze(const Polygon &p_other) const
{
	if (isCoplanar(p_other) == false)
	{
		GENERATE_ERROR("Polygons must be coplanar");
	}

	if (isAdjacent(p_other) == false && isOverlapping(p_other) == false)
	{
		return tmp::Polygon();
	}

	spk::Vector3 n = normal();

	std::vector<tmp::Edge> A = _edges;
	std::vector<tmp::Edge> B = p_other.edges();

	splitAllEdges(A, B, n);
	splitAllEdges(B, A, n);

	std::vector<tmp::Edge> boundary = subtractInternalShared(A, B);
	if (boundary.empty())
	{
		GENERATE_ERROR("Fused polygon is empty or degenerate");
	}

	std::vector<spk::Vector3> loop = stitchLoop(boundary, n);
	if (loop.size() < 4)
	{
		GENERATE_ERROR("Fused polygon could not be stitched");
	}

	return tmp::Polygon::fromLoop(loop);
}

class CollisionMesh
{
public:
	using Unit = tmp::Polygon;

private:
	std::vector<Unit> _units;

public:
	CollisionMesh() = default;

	void addUnit(const Unit &p_unit)
	{
		_units.push_back(p_unit);
	}

	const std::vector<Unit> &units() const
	{
		return (_units);
	}

	static CollisionMesh fromObjMesh(const spk::SafePointer<spk::ObjMesh> &p_mesh)
	{
		CollisionMesh result;
		for (const auto &shapeVariant : p_mesh->shapes())
		{
			Unit poly;
			if (std::holds_alternative<spk::ObjMesh::Quad>(shapeVariant) == true)
			{
				const auto &q = std::get<spk::ObjMesh::Quad>(shapeVariant);
				poly = tmp::Polygon::makeSquare(q.a.position, q.b.position, q.c.position, q.d.position);
			}
			else
			{
				const auto &t = std::get<spk::ObjMesh::Triangle>(shapeVariant);
				poly = tmp::Polygon::makeTriangle(t.a.position, t.b.position, t.c.position);
			}

			bool removed = false;
			for (auto it = result._units.begin(); it != result._units.end(); ++it)
			{
				if (it->isCoplanar(poly) == true && it->normal() == poly.normal().inverse())
				{
					bool shared = it->isOverlapping(poly);
					if (shared == false)
					{
						for (const auto &edgeA : it->edges())
						{
							for (const auto &edgeB : poly.edges())
							{
								if (edgeA.isInverse(edgeB) == true)
								{
									shared = true;
									break;
								}
							}
							if (shared == true)
							{
								break;
							}
						}
					}
					if (shared == true)
					{
						result._units.erase(it);
						removed = true;
						break;
					}
				}
			}
			if (removed == true)
			{
				continue;
			}

			bool fused = false;
			for (auto &existing : result._units)
			{
				if (existing.isCoplanar(poly) == true && existing.normal() == poly.normal() &&
					(existing.isAdjacent(poly) == true || existing.isOverlapping(poly) == true))
				{
					existing = existing.fuze(poly);
					fused = true;
					break;
				}
			}
			if (fused == false)
			{
				result.addUnit(poly);
			}
		}
		return (result);
	}
};
int main()
{
	spk::ObjMesh mesh = spk::ObjMesh::loadFromFile("playground/resources/obj/two_cubes.obj");
	tmp::CollisionMesh collision = tmp::CollisionMesh::fromObjMesh(&mesh);

	for (const auto &poly : collision.units())
	{
		std::cout << poly << std::endl;
	}

	return (0);
}
