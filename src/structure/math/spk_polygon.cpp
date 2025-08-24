#include "structure/math/spk_polygon.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

#include "spk_constants.hpp"
#include "spk_debug_macro.hpp"

namespace spk
{
	float Polygon::_polyScale(const std::vector<spk::Vector3> &p_points)
	{
		float m = 0.0f;
		for (size_t i = 1; i < p_points.size(); ++i)
		{
			m = std::max(m, (p_points[i] - p_points[i - 1]).norm());
		}
		return (m > 0.0f ? m : 1.0f);
	}

	bool Polygon::_approxEq(float p_a, float p_b, float p_tol)
	{
		return std::fabs(p_a - p_b) <= p_tol;
	}

	bool Polygon::_inRange(float p_x, float p_a, float p_b, float p_tol)
	{
		const float lo = std::min(p_a, p_b) - p_tol;
		const float hi = std::max(p_a, p_b) + p_tol;
		return (p_x >= lo && p_x <= hi);
	}

	bool Polygon::_pointOnSegment2D(const spk::Vector2 &p_p, const spk::Vector2 &p_a, const spk::Vector2 &p_b, float p_tol)
	{
		const spk::Vector2 AB = p_b - p_a;
		const spk::Vector2 AP = p_p - p_a;
		const float cross = AB.x * AP.y - AB.y * AP.x;
		if (std::fabs(cross) > p_tol)
		{
			return false;
		}

		const float dot = AP.x * AB.x + AP.y * AB.y;
		if (dot < -p_tol)
		{
			return false;
		}

		const float len2 = AB.x * AB.x + AB.y * AB.y;
		if (dot > len2 + p_tol)
		{
			return false;
		}

		return true;
	}

	void Polygon::_insertEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b)
	{
		if (_hasEdge(p_a, p_b) == true)
		{
			_removeEdge(p_a, p_b);
			return;
		}
		if (_hasEdge(p_b, p_a) == true)
		{
			_removeEdge(p_b, p_a);
			return;
		}
		_edges[p_a].push_back(p_b);
		_edges[p_b].push_back(p_a);
	}

	bool Polygon::_hasEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b) const
	{
		auto it = _edges.find(p_a);
		if (it == _edges.end())
		{
			return false;
		}
		return std::find(it->second.begin(), it->second.end(), p_b) != it->second.end();
	}

	void Polygon::_removeEdge(const spk::Vector3 &p_a, const spk::Vector3 &p_b)
	{
		auto eraseFrom =
			[](std::unordered_map<spk::Vector3, std::vector<spk::Vector3>> &p_map, const spk::Vector3 &p_start, const spk::Vector3 &p_end)
		{
			if (auto it = p_map.find(p_start); it != p_map.end())
			{
				auto &vec = it->second;
				vec.erase(std::remove(vec.begin(), vec.end(), p_end), vec.end());
				if (vec.empty() == true)
				{
					p_map.erase(it);
				}
			}
		};

		eraseFrom(_edges, p_a, p_b);
		eraseFrom(_edges, p_b, p_a);
	}

	void Polygon::_computeWire()
	{
		std::vector<spk::Vector3> allPts;
		for (const auto &kv : _edges)
		{
			allPts.push_back(kv.first);
			for (const auto &dst : kv.second)
			{
				allPts.push_back(dst);
			}
		}
		if (allPts.empty() == true)
		{
			_points.clear();
			return;
		}

		auto cmp = [](const spk::Vector3 &p_a, const spk::Vector3 &p_b)
		{
			if (p_a.x == p_b.x)
			{
				return p_a.y < p_b.y;
			}
			return p_a.x < p_b.x;
		};

		spk::Vector3 start = *std::min_element(allPts.begin(), allPts.end(), cmp);
		spk::Vector3 current = start;
		spk::Vector3 prevDir{1.0f, 0.0f, 0.0f};
		std::unordered_map<spk::Vector3, std::vector<spk::Vector3>> edgesCopy = _edges;
		_points.clear();

		auto removeEdgeFrom = [](auto &p_map, const spk::Vector3 &p_a, const spk::Vector3 &p_b)
		{
			if (auto it = p_map.find(p_a); it != p_map.end())
			{
				auto &vec = it->second;
				vec.erase(std::remove(vec.begin(), vec.end(), p_b), vec.end());
				if (vec.empty() == true)
				{
					p_map.erase(it);
				}
			}
		};

		do
		{
			_points.push_back(current);
			auto &neighbors = edgesCopy[current];
			if (neighbors.empty() == true)
			{
				break;
			}
			spk::Vector3 next = neighbors.front();
			float bestAngle = std::numeric_limits<float>::max();
			for (const auto &cand : neighbors)
			{
				spk::Vector3 dir = (cand - current).normalize();
				float angle = std::atan2(prevDir.cross(dir).z, prevDir.dot(dir));
				if (angle < 0.0f)
				{
					angle += 2.0f * spk::Constants::pi;
				}
				if (angle < bestAngle)
				{
					bestAngle = angle;
					next = cand;
				}
			}
			removeEdgeFrom(edgesCopy, current, next);
			removeEdgeFrom(edgesCopy, next, current);
			prevDir = (next - current).normalize();
			current = next;
		} while ((current == start) == false);

		_edges.clear();
		for (size_t i = 0; i < _points.size(); ++i)
		{
			const spk::Vector3 &a = _points[i];
			const spk::Vector3 &b = _points[(i + 1) % _points.size()];
			_edges[a].push_back(b);
			_edges[b].push_back(a);
		}
	}

	const std::vector<spk::Vector3> &Polygon::points() const
	{
		return _points;
	}

	std::vector<spk::Vector3> Polygon::rewind() const
	{
		return _points;
	}

	bool Polygon::canInsert(const Polygon &p_polygon) const
	{
		for (const auto &kv : p_polygon._edges)
		{
			for (const auto &dst : kv.second)
			{
				if ((_hasEdge(kv.first, dst) == true) || (_hasEdge(dst, kv.first) == true))
				{
					return true;
				}
			}
		}
		return false;
	}

	void Polygon::addTriangle(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c)
	{
		_insertEdge(p_a, p_b);
		_insertEdge(p_b, p_c);
		_insertEdge(p_c, p_a);
		_computeWire();
	}

	void Polygon::addQuad(const spk::Vector3 &p_a, const spk::Vector3 &p_b, const spk::Vector3 &p_c, const spk::Vector3 &p_d)
	{
		_insertEdge(p_a, p_b);
		_insertEdge(p_b, p_c);
		_insertEdge(p_c, p_d);
		_insertEdge(p_d, p_a);
		_computeWire();
	}

	void Polygon::addPolygon(const Polygon &p_polygon)
	{
		if (_edges.empty() == true)
		{
			_edges = p_polygon._edges;
			_computeWire();
			return;
		}
		if (canInsert(p_polygon) == false)
		{
			GENERATE_ERROR("Polygon cannot be inserted");
		}
		for (const auto &kv : p_polygon._edges)
		{
			for (const auto &dst : kv.second)
			{
				_insertEdge(kv.first, dst);
			}
		}
		_computeWire();
	}

	bool Polygon::isConvex() const
	{
		if (_points.size() < 3)
		{
			return false;
		}
		spk::Vector3 normal = (_points[1] - _points[0]).cross(_points[2] - _points[0]);
		for (size_t i = 0; i < _points.size(); ++i)
		{
			const spk::Vector3 &a = _points[i];
			const spk::Vector3 &b = _points[(i + 1) % _points.size()];
			const spk::Vector3 &c = _points[(i + 2) % _points.size()];
			if (((b - a).cross(c - b)).dot(normal) < -spk::Constants::pointPrecision)
			{
				return false;
			}
		}
		return true;
	}

	std::vector<Polygon> Polygon::triangulize() const
	{
		if (isConvex() == false)
		{
			GENERATE_ERROR("Polygon not convex");
		}
		std::vector<Polygon> result;
		for (size_t i = 1; i + 1 < _points.size(); ++i)
		{
			Polygon tri;
			tri.addTriangle(_points[0], _points[i], _points[i + 1]);
			result.push_back(tri);
		}
		return result;
	}

	std::vector<Polygon> Polygon::split() const
	{
		if (isConvex() == true)
		{
			return {*this};
		}

		std::vector<Polygon> result;
		if (_points.size() < 3)
		{
			return result;
		}

		const spk::Vector3 &origin = _points[0];
		spk::Vector3 u = (_points[1] - origin).normalize();
		spk::Vector3 normal = (_points[1] - origin).cross(_points[2] - origin).normalize();
		spk::Vector3 v = normal.cross(u);

		std::vector<spk::Vector2> pts2d;
		pts2d.reserve(_points.size());
		for (const spk::Vector3 &pt : _points)
		{
			spk::Vector3 rel = pt - origin;
			pts2d.push_back(spk::Vector2(rel.dot(u), rel.dot(v)));
		}

		auto cross2d = [](const spk::Vector2 &p_a, const spk::Vector2 &p_b, const spk::Vector2 &p_c)
		{
			spk::Vector2 ab = p_b - p_a;
			spk::Vector2 ac = p_c - p_a;
			return ab.x * ac.y - ab.y * ac.x;
		};

		auto pointInTri = [&](const spk::Vector2 &p_p, const spk::Vector2 &p_a, const spk::Vector2 &p_b, const spk::Vector2 &p_c)
		{
			float c1 = cross2d(p_a, p_b, p_p);
			float c2 = cross2d(p_b, p_c, p_p);
			float c3 = cross2d(p_c, p_a, p_p);
			bool hasNeg = (c1 < -spk::Constants::pointPrecision) || (c2 < -spk::Constants::pointPrecision) || (c3 < -spk::Constants::pointPrecision);
			bool hasPos = (c1 > spk::Constants::pointPrecision) || (c2 > spk::Constants::pointPrecision) || (c3 > spk::Constants::pointPrecision);
			return !(hasNeg && hasPos);
		};

		std::vector<size_t> idx(_points.size());
		std::iota(idx.begin(), idx.end(), 0);

		while (idx.size() >= 3)
		{
			bool earFound = false;
			for (size_t i = 0; i < idx.size(); ++i)
			{
				size_t prev = idx[(i + idx.size() - 1) % idx.size()];
				size_t curr = idx[i];
				size_t next = idx[(i + 1) % idx.size()];

				const spk::Vector2 &A = pts2d[prev];
				const spk::Vector2 &B = pts2d[curr];
				const spk::Vector2 &C = pts2d[next];

				if (cross2d(A, B, C) <= spk::Constants::pointPrecision)
				{
					continue;
				}

				bool containsPt = false;
				for (size_t k = 0; k < idx.size(); ++k)
				{
					size_t vi = idx[k];
					if (vi == prev || vi == curr || vi == next)
					{
						continue;
					}
					if (pointInTri(pts2d[vi], A, B, C) == true)
					{
						containsPt = true;
						break;
					}
				}

				if (containsPt == true)
				{
					continue;
				}

				Polygon tri;
				tri.addTriangle(_points[prev], _points[curr], _points[next]);
				result.push_back(tri);
				idx.erase(idx.begin() + i);
				earFound = true;
				break;
			}
			if (earFound == false)
			{
				break;
			}
		}

		return result;
	}
}
