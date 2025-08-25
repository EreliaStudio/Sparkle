#include <sparkle.hpp>

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
			for (size_t i = 1; i < p_vs.size(); ++i)
			{
				r._addEdge(p_vs[i - 1], p_vs[i]);
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
	float clampf(float v, float lo, float hi)
	{
		return std::max(lo, std::min(v, hi));
	}

	// Gather split params along 'base' when 'other' is colinear and overlapping
	void collectSplitTsForColinearOverlap(const tmp::Edge &base, const tmp::Edge &other, std::vector<float> &ts)
	{
		if (base.isColinear(other) == false)
		{
			return;
		}

		const float len2 = base.delta().dot(base.delta());
		if (FLOAT_EQ(len2, 0.0f))
		{
			return;
		}

		float t0 = base.project(other.first());
		float t1 = base.project(other.second());
		if (t1 < t0)
		{
			std::swap(t0, t1);
		}

		float lo = clampf(t0, 0.0f, len2);
		float hi = clampf(t1, 0.0f, len2);

		if (FLOAT_NEQ(hi - lo, 0.0f))
		{
			ts.push_back(lo);
			ts.push_back(hi);
		}
	}

	// Split one edge by collected ts
	void splitEdgeByTs(const tmp::Edge &e, const std::vector<float> &ts, std::vector<tmp::Edge> &out)
	{
		const float len = e.delta().norm();
		const float len2 = len * len;
		if (FLOAT_EQ(len, 0.0f))
		{
			return;
		}

		std::vector<float> cuts;
		cuts.reserve(ts.size() + 2);
		cuts.push_back(0.0f);
		for (float t : ts)
		{
			if (t > 0.0f && t < len2)
			{
				cuts.push_back(t);
			}
		}
		cuts.push_back(len2);
		std::sort(cuts.begin(), cuts.end());
		cuts.erase(std::unique(cuts.begin(), cuts.end(), [](float a, float b) { return FLOAT_EQ(a, b); }), cuts.end());

		for (size_t i = 1; i < cuts.size(); ++i)
		{
			float t0 = cuts[i - 1], t1 = cuts[i];
			if (FLOAT_EQ(t1 - t0, 0.0f))
			{
				continue;
			}

			const spk::Vector3 a = e.first() + e.direction() * (t0 / len);
			const spk::Vector3 b = e.first() + e.direction() * (t1 / len);
			out.emplace_back(a, b);
		}
	}

	// Split all edges of base by colinear overlaps with other
	void splitAllEdges(std::vector<tmp::Edge> &base, const std::vector<tmp::Edge> &other)
	{
		std::vector<tmp::Edge> result;
		result.reserve(base.size() * 2);
		for (const auto &e : base)
		{
			std::vector<float> ts;
			for (const auto &o : other)
			{
				collectSplitTsForColinearOverlap(e, o, ts);
			}
			splitEdgeByTs(e, ts, result);
		}
		base.swap(result);
	}

	// Remove sub-edges that appear once in each set (use Edge::isSame directly)
	std::vector<tmp::Edge> subtractInternalShared(const std::vector<tmp::Edge> &A, const std::vector<tmp::Edge> &B)
	{
		std::vector<bool> usedB(B.size(), false);
		std::vector<tmp::Edge> out;
		out.reserve(A.size() + B.size());

		for (size_t i = 0; i < A.size(); ++i)
		{
			bool matched = false;
			for (size_t j = 0; j < B.size(); ++j)
			{
				if (usedB[j])
				{
					continue;
				}
				if (A[i].isSame(B[j]))
				{
					usedB[j] = true;
					matched = true;
					break;
				}
			}
			if (!matched)
			{
				out.push_back(A[i]);
			}
		}
		for (size_t j = 0; j < B.size(); ++j)
		{
			if (!usedB[j])
			{
				out.push_back(B[j]);
			}
		}
		return out;
	}

	// Build a polygon loop from boundary edges (same as before)
	std::vector<spk::Vector3> stitchLoop(const std::vector<tmp::Edge> &edges)
	{
		if (edges.empty())
		{
			return {};
		}

		std::map<spk::Vector3, std::vector<size_t>> outAdj, inAdj;
		for (size_t i = 0; i < edges.size(); ++i)
		{
			outAdj[edges[i].first()].push_back(i);
			inAdj[edges[i].second()].push_back(i);
		}

		spk::Vector3 start = edges[0].first();
		for (const auto &e : edges)
		{
			if (e.first() < start)
			{
				start = e.first();
			}
			if (e.second() < start)
			{
				start = e.second();
			}
		}

		std::vector<bool> used(edges.size(), false);
		std::vector<spk::Vector3> loop;
		loop.reserve(edges.size() + 1);
		spk::Vector3 cur = start;
		loop.push_back(cur);

		for (;;)
		{
			size_t pick = SIZE_MAX;
			bool forward = true;

			if (auto it = outAdj.find(cur); it != outAdj.end())
			{
				for (size_t idx : it->second)
				{
					if (!used[idx])
					{
						pick = idx;
						forward = true;
						break;
					}
				}
			}
			if (pick == SIZE_MAX)
			{
				if (auto it = inAdj.find(cur); it != inAdj.end())
				{
					for (size_t idx : it->second)
					{
						if (!used[idx])
						{
							pick = idx;
							forward = false;
							break;
						}
					}
				}
			}

			if (pick == SIZE_MAX)
			{
				break;
			}

			used[pick] = true;
			cur = forward ? edges[pick].second() : edges[pick].first();

			if (loop.size() > 1 && (cur == loop.front()))
			{
				break;
			}
			loop.push_back(cur);
		}

		if (loop.size() >= 3 && !(loop.front() == loop.back()))
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

	// Refuse real overlaps (crossing edges not colinear)
	spk::Vector3 n = normal();
	for (const auto &ea : _edges)
	{
		for (const auto &eb : p_other.edges())
		{
			if (_edgesIntersect(ea, eb, n, spk::Constants::pointPrecision))
			{
				GENERATE_ERROR("Union for overlapping (non-colinear) polygons not implemented yet");
			}
		}
	}

	std::vector<tmp::Edge> A = _edges;
	std::vector<tmp::Edge> B = p_other.edges();

	splitAllEdges(A, B);
	splitAllEdges(B, A);

	std::vector<tmp::Edge> boundary = subtractInternalShared(A, B);
	if (boundary.empty())
	{
		GENERATE_ERROR("Fused polygon is empty or degenerate");
	}

	std::vector<spk::Vector3> loop = stitchLoop(boundary);
	if (loop.size() < 4)
	{
		GENERATE_ERROR("Fused polygon could not be stitched");
	}

	return tmp::Polygon::fromLoop(loop);
}

class PolygonOutputer
{
private:
	spk::Vector3Int _min;
	spk::Vector3Int _max = 0;

	std::vector<tmp::Polygon> _polygons;

public:
	void addPolygon(const tmp::Polygon &p_polygon)
	{
		_polygons.push_back(p_polygon);
		for (const auto &edge : p_polygon.edges())
		{
			_min = spk::Vector3Int::min(_min, spk::Vector3Int::min(edge.first(), edge.second()));
			_max = spk::Vector3Int::max(_max, spk::Vector3Int::max(edge.first(), edge.second()));
		}
	}

	void print() const
	{
		if (_polygons.empty() || _max == 0)
		{
			std::cout << "(no polygons)" << std::endl;
			return;
		}

		int width = _max.x - _min.x + 1;
		int height = _max.y - _min.y + 1;

		// Start with a blank canvas
		std::vector<std::string> canvas(height, std::string(width, ' '));

		// Draw polygons
		char polyChar = 'a';
		for (size_t p = 0; p < _polygons.size(); ++p, ++polyChar)
		{
			for (const auto &edge : _polygons[p].edges())
			{
				spk::Vector3Int a = spk::Vector3Int(edge.first());
				spk::Vector3Int b = spk::Vector3Int(edge.second());

				// Bresenham line drawing in integer grid
				int x0 = a.x - _min.x;
				int y0 = a.y - _min.y;
				int x1 = b.x - _min.x;
				int y1 = b.y - _min.y;

				int dx = std::abs(x1 - x0);
				int dy = -std::abs(y1 - y0);
				int sx = (x0 < x1) ? 1 : -1;
				int sy = (y0 < y1) ? 1 : -1;
				int err = dx + dy;

				while (true)
				{
					if (y0 >= 0 && y0 < height && x0 >= 0 && x0 < width)
					{
						canvas[height - 1 - y0][x0] = polyChar;
					}
					if (x0 == x1 && y0 == y1)
					{
						break;
					}
					int e2 = 2 * err;
					if (e2 >= dy)
					{
						err += dy;
						x0 += sx;
					}
					if (e2 <= dx)
					{
						err += dx;
						y0 += sy;
					}
				}
			}
		}

		// Print canvas
		for (const auto &row : canvas)
		{
			std::cout << row << "\n";
		}
	}
};

void checkPolygons(std::string_view p_nameA, const tmp::Polygon &p_polyA, std::string_view p_nameB, const tmp::Polygon &p_polyB)
{
	std::cout << " ----- " << p_nameA << " -> " << p_nameB << " -----" << std::endl;
	PolygonOutputer tmp;
	tmp.addPolygon(p_polyA);
	tmp.addPolygon(p_polyB);
	tmp.print();
	std::cout << " ----- ------ -----" << std::endl << std::endl;

	std::cout << "Is planar " << p_nameA << " : " << std::boolalpha << p_polyA.isPlanar() << std::endl;
	std::cout << "Is planar " << p_nameB << " : " << std::boolalpha << p_polyB.isPlanar() << std::endl;
	std::cout << "Is Coplanar : " << std::boolalpha << p_polyA.isCoplanar(p_polyB) << std::endl;
	std::cout << "Is adjacent : " << std::boolalpha << p_polyA.isAdjacent(p_polyB) << std::endl;
	std::cout << "Is overlapping : " << std::boolalpha << p_polyA.isOverlapping(p_polyB) << std::endl;

	std::cout << " ----- Fused  -----" << std::endl << std::endl;
	try
	{
		PolygonOutputer tmp;
		tmp.addPolygon(p_polyA.fuze(p_polyB));
		tmp.print();
	} catch (const std::exception &e)
	{
		std::cout << e.what() << std::endl;
	} catch (...)
	{
		std::cout << "Can't be fused : unknow error" << std::endl;
	}
	std::cout << " ----- ------ -----" << std::endl << std::endl;
}

int main()
{
	std::array<spk::Vector3, 4> points = {spk::Vector3{0, 0, 0}, spk::Vector3{10, 0, 0}, spk::Vector3{10, 10, 0}, spk::Vector3{0, 10, 0}};
	spk::Vector3 deltaB = {10, 0, 0};
	spk::Vector3 deltaC = {5, 10, 0};
	spk::Vector3 deltaD = {5, 5, 0};
	spk::Vector3 deltaE = {15, 5, 0};
	tmp::Polygon polyA = tmp::Polygon::makeSquare(points[0], points[1], points[2], points[3]);
	tmp::Polygon polyB = tmp::Polygon::makeSquare(points[0] + deltaB, points[1] + deltaB, points[2] + deltaB, points[3] + deltaB);
	tmp::Polygon polyC = tmp::Polygon::makeSquare(points[0] + deltaC, points[1] + deltaC, points[2] + deltaC, points[3] + deltaC);
	tmp::Polygon polyD = tmp::Polygon::makeSquare(points[0] + deltaD, points[1] + deltaD, points[2] + deltaD, points[3] + deltaD);
	tmp::Polygon polyE = tmp::Polygon::makeSquare(points[0] + deltaE, points[1] + deltaE, points[2] + deltaE, points[3] + deltaE);

	checkPolygons("A", polyA, "B", polyB);
	checkPolygons("A", polyA, "C", polyC);
	checkPolygons("A", polyA, "D", polyD);
	checkPolygons("A", polyA, "E", polyE);

	return (0);
}
