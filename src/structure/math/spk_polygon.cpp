#include "structure/math/spk_polygon.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>

#include "spk_constants.hpp"
#include "spk_debug_macro.hpp"

namespace spk
{
	float Polygon::_polyScale(const std::vector<spk::Vector3> &p_points)
	{
		float m = 0.f;
		for (size_t i = 1; i < p_points.size(); ++i)
		{
			m = std::max(m, (p_points[i] - p_points[i - 1]).norm());
		}
		return (m > 0.f ? m : 1.f);
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

	spk::Vector3 Polygon::normal() const
	{
		if (points.size() < 3)
		{
			GENERATE_ERROR("Can't generate normal on a polygon with less than 3 points");
		}
		const spk::Vector3 &origin = points[0];
		spk::Vector3 normal = (points[1] - origin).cross(points[2] - origin);
		return (normal.normalize());
	}

	bool Polygon::contains(const spk::Vector3 &p_point) const
	{
		if (points.size() < 3)
		{
			return false;
		}
		const spk::Vector3 &origin = points[0];
		spk::Vector3 n = normal();
		spk::Vector3 u = (points[1] - origin).normalize();
		spk::Vector3 v = n.cross(u);

		float tol = spk::Constants::pointPrecision * _polyScale(points);

		spk::Vector3 rel = p_point - origin;
		spk::Vector2 P(rel.dot(u), rel.dot(v));

		std::vector<spk::Vector2> proj;
		proj.reserve(points.size());
		for (const spk::Vector3 &pt : points)
		{
			spk::Vector3 r = pt - origin;
			proj.emplace_back(r.dot(u), r.dot(v));
		}

		bool inside = false;
		for (size_t i = 0, j = proj.size() - 1; i < proj.size(); j = i++)
		{
			const spk::Vector2 &A = proj[i];
			const spk::Vector2 &B = proj[j];
			if (_pointOnSegment2D(P, A, B, tol) == true)
			{
				return true;
			}
			bool intersect =
				((A.y > P.y) != (B.y > P.y)) && (P.x < (B.x - A.x) * (P.y - A.y) / (B.y - A.y + std::numeric_limits<float>::epsilon()) + A.x);
			if (intersect == true)
			{
				inside = (inside == false);
			}
		}
		return inside;
	}

	bool Polygon::contains(const Polygon &p_polygon) const
	{
		for (const spk::Vector3 &pt : p_polygon.points)
		{
			bool inside = contains(pt);
			if (inside == false)
			{
				return false;
			}
		}
		return true;
	}

	bool Polygon::isCoplanar(const Polygon &p_polygon) const
	{
		if (points.size() < 3 || p_polygon.points.size() < 3)
		{
			return false;
		}
		const float tol = spk::Constants::pointPrecision * std::max(_polyScale(points), _polyScale(p_polygon.points));

		spk::Vector3 n = normal();
		spk::Vector3 otherNormal = p_polygon.normal();

		bool sameNormal = n == otherNormal;
		bool oppositeNormal = n == -otherNormal;

		if (sameNormal == false && oppositeNormal == false)
		{
			return false;
		}

		const spk::Vector3 &origin = points[0];
		for (const spk::Vector3 &pt : p_polygon.points)
		{
			float d = n.dot(pt - origin);
			if (std::fabs(d) > tol)
			{
				return false;
			}
		}
		return true;
	}

	bool Polygon::isAdjacent(const Polygon &p_polygon) const
	{
		if (isCoplanar(p_polygon) == false)
		{
			return false;
		}

		std::unordered_map<spk::Vector3, size_t> pointMap;
		pointMap.reserve(p_polygon.points.size());
		for (size_t j = 0; j < p_polygon.points.size(); ++j)
		{
			pointMap[p_polygon.points[j]] = j;
		}

		for (size_t i = 0; i < points.size(); ++i)
		{
			size_t nextI = (i + 1) % points.size();

			auto it = pointMap.find(points[i]);
			if (it != pointMap.end())
			{
				size_t j = it->second;
				size_t nextJ = (j + 1) % p_polygon.points.size();
				size_t prevJ = (j + p_polygon.points.size() - 1) % p_polygon.points.size();
				if (points[nextI] == p_polygon.points[nextJ] || points[nextI] == p_polygon.points[prevJ])
				{
					return true;
				}
			}

			it = pointMap.find(points[nextI]);
			if (it != pointMap.end())
			{
				size_t j = it->second;
				size_t nextJ = (j + 1) % p_polygon.points.size();
				size_t prevJ = (j + p_polygon.points.size() - 1) % p_polygon.points.size();
				if (points[i] == p_polygon.points[nextJ] || points[i] == p_polygon.points[prevJ])
				{
					return true;
				}
			}
		}
		return false;
	}

	bool Polygon::isMergable(const Polygon &p_polygon) const
	{
		bool coplanar = isCoplanar(p_polygon);
		bool adjacent = isAdjacent(p_polygon);
		return (coplanar == true && adjacent == true);
	}

	void Polygon::rewind(Winding p_winding)
	{
		if (points.size() < 3)
		{
			return;
		}
		spk::Vector3 centroid(0.f, 0.f, 0.f);
		for (const spk::Vector3 &pt : points)
		{
			centroid += pt;
		}
		centroid /= static_cast<float>(points.size());

		spk::Vector3 u = (points[0] - centroid).normalize();
		spk::Vector3 normal = (points[1] - centroid).cross(points[2] - centroid).normalize();
		spk::Vector3 v = normal.cross(u);

		std::sort(
			points.begin(),
			points.end(),
			[&](const spk::Vector3 &p_a, const spk::Vector3 &p_b)
			{
				spk::Vector3 ra = p_a - centroid;
				spk::Vector3 rb = p_b - centroid;
				float angleA = std::atan2(ra.dot(v), ra.dot(u));
				float angleB = std::atan2(rb.dot(v), rb.dot(u));
				return angleA < angleB;
			});

		if (p_winding == Winding::Clockwise)
		{
			std::reverse(points.begin(), points.end());
		}
	}

	bool Polygon::isConvex() const
	{
		if (points.size() < 3)
		{
			return false;
		}
		spk::Vector3 n = normal();
		bool hasPos = false;
		bool hasNeg = false;
		for (size_t i = 0; i < points.size(); ++i)
		{
			const spk::Vector3 &A = points[i];
			const spk::Vector3 &B = points[(i + 1) % points.size()];
			const spk::Vector3 &C = points[(i + 2) % points.size()];
			float val = (B - A).cross(C - B).dot(n);
			if (val > spk::Constants::pointPrecision)
			{
				hasPos = true;
			}
			else if (val < -spk::Constants::pointPrecision)
			{
				hasNeg = true;
			}
			if (hasPos == true && hasNeg == true)
			{
				return false;
			}
		}
		return true;
	}

	std::vector<Polygon> Polygon::triangulize() const
	{
		std::vector<Polygon> result;
		if (points.size() < 3)
		{
			return result;
		}
		for (size_t i = 1; i + 1 < points.size(); ++i)
		{
			Polygon tri;
			tri.points = {points[0], points[i], points[i + 1]};
			result.push_back(tri);
		}
		return result;
	}

	void Polygon::merge(const Polygon &p_polygon)
	{
		if (isMergable(p_polygon) == false)
		{
			return;
		}
		std::vector<spk::Vector3> allPts = points;
		for (const spk::Vector3 &pt : p_polygon.points)
		{
			allPts.push_back(pt);
		}

		const spk::Vector3 &origin = allPts[0];
		spk::Vector3 u = (allPts[1] - origin).normalize();
		spk::Vector3 normal = (allPts[1] - origin).cross(allPts[2] - origin).normalize();
		spk::Vector3 v = normal.cross(u);

		std::vector<std::pair<spk::Vector2, size_t>> proj;
		proj.reserve(allPts.size());
		for (size_t i = 0; i < allPts.size(); ++i)
		{
			spk::Vector3 rel = allPts[i] - origin;
			proj.emplace_back(spk::Vector2(rel.dot(u), rel.dot(v)), i);
		}

		std::sort(
			proj.begin(),
			proj.end(),
			[](const auto &a, const auto &b)
			{
				if (a.first.x != b.first.x)
				{
					return a.first.x < b.first.x;
				}
				return a.first.y < b.first.y;
			});

		std::vector<size_t> hull;
		hull.reserve(proj.size() * 2);
		for (const auto &p : proj)
		{
			while (hull.size() >= 2)
			{
				const spk::Vector2 &A = proj[hull[hull.size() - 2]].first;
				const spk::Vector2 &B = proj[hull.back()].first;
				const spk::Vector2 &C = p.first;
				spk::Vector2 ab = B - A;
				spk::Vector2 ac = C - A;
				if (ab.x * ac.y - ab.y * ac.x <= spk::Constants::pointPrecision)
				{
					hull.pop_back();
				}
				else
				{
					break;
				}
			}
			hull.push_back(&p - &proj[0]);
		}
		for (int i = static_cast<int>(proj.size()) - 2; i >= 0; --i)
		{
			const auto &p = proj[i];
			while (hull.size() >= 2)
			{
				const spk::Vector2 &A = proj[hull[hull.size() - 2]].first;
				const spk::Vector2 &B = proj[hull.back()].first;
				const spk::Vector2 &C = p.first;
				spk::Vector2 ab = B - A;
				spk::Vector2 ac = C - A;
				if (ab.x * ac.y - ab.y * ac.x <= spk::Constants::pointPrecision)
				{
					hull.pop_back();
				}
				else
				{
					break;
				}
			}
			hull.push_back(&p - &proj[0]);
		}
		hull.pop_back();

		std::vector<spk::Vector3> merged;
		merged.reserve(hull.size());
		for (size_t idx : hull)
		{
			merged.push_back(allPts[proj[idx].second]);
		}
		points = std::move(merged);
		rewind(Winding::CounterClockwise);
	}

	std::vector<Polygon> Polygon::split() const
	{
		std::vector<Polygon> result;
		if (points.size() < 3)
		{
			return result;
		}

		const spk::Vector3 &origin = points[0];
		spk::Vector3 u = (points[1] - origin).normalize();
		spk::Vector3 normal = (points[1] - origin).cross(points[2] - origin).normalize();
		spk::Vector3 v = normal.cross(u);

		std::vector<spk::Vector2> pts2d;
		pts2d.reserve(points.size());
		for (const spk::Vector3 &pt : points)
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

		std::vector<size_t> idx(points.size());
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
				tri.points = {points[prev], points[curr], points[next]};
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
		rewind(Winding::CounterClockwise);
		return result;
	}
}
