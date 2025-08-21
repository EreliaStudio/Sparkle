#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "spk_debug_macro.hpp"

namespace spk
{
	struct Polygon
	{
	private:
		static float _polyScale(const std::vector<spk::Vector3> &ps)
		{
			float m = 0.f;
			for (size_t i = 1; i < ps.size(); ++i)
			{
				m = std::max(m, (ps[i] - ps[i - 1]).norm());
			}
			return (m > 0.f ? m : 1.f);
		}

		static bool _approxEq(float a, float b, float tol)
		{
			return std::fabs(a - b) <= tol;
		}

		static bool _inRange(float x, float a, float b, float tol)
		{
			const float lo = std::min(a, b) - tol;
			const float hi = std::max(a, b) + tol;
			return (x >= lo && x <= hi);
		}

		static bool _pointOnSegment2D(const spk::Vector2 &P, const spk::Vector2 &A, const spk::Vector2 &B, float tol)
		{
			// Collinearity via cross, then projection within segment via dot
			const spk::Vector2 AB = B - A;
			const spk::Vector2 AP = P - A;
			const float cross = AB.x * AP.y - AB.y * AP.x;
			if (std::fabs(cross) > tol)
			{
				return false;
			}

			const float dot = AP.x * AB.x + AP.y * AB.y;
			if (dot < -tol)
			{
				return false;
			}

			const float len2 = AB.x * AB.x + AB.y * AB.y;
			if (dot > len2 + tol)
			{
				return false;
			}

			return true;
		}

	public:
		std::vector<spk::Vector3> points;

		spk::Vector3 normal() const
		{
			if (points.size() < 3)
			{
				GENERATE_ERROR("Can't generate normal on a polygon with less than 3 points");
			}
			const spk::Vector3 &origin = points[0];
			spk::Vector3 normal = (points[1] - origin).cross(points[2] - origin);
			return (normal.normalize());
		}

		bool isPlanar() const
		{
			if (points.size() < 4)
			{
				return true;
			}
			const float tol = 1e-6f * _polyScale(points);

			const spk::Vector3 &origin = points[0];
			spk::Vector3 n = normal();
			for (size_t i = 1; i < points.size(); ++i)
			{
				if (std::fabs(n.dot(points[i] - origin)) > tol)
				{
					return false;
				}
			}
			return true;
		}

		bool isCoplanar(const Polygon &p_polygon) const
		{
			if (isPlanar() == false || p_polygon.isPlanar() == false)
			{
				return false;
			}

			spk::Vector3 n = normal();
			spk::Vector3 otherNormal = p_polygon.normal();

			bool sameNormal = n == otherNormal;
			bool oppositeNormal = n == -otherNormal;

			if (sameNormal == false && oppositeNormal == false)
			{
				return false;
			}

			const spk::Vector3 &origin = points[0];
			float distance = n.dot(p_polygon.points[0] - origin);
			bool result = std::abs(distance) <= std::numeric_limits<float>::epsilon();
			return result;
		}

		bool contains(const spk::Vector3 &p_point) const
		{
			if (points.size() < 3)
			{
				return false;
			}

			const float scale = _polyScale(points);
			const float tol3D = 1e-6f * scale;
			const float tol2D = 1e-6f * std::max(1.0f, scale);

			const spk::Vector3 &origin = points[0];
			spk::Vector3 u = (points[1] - origin).normalize();
			spk::Vector3 normal = (points[1] - origin).cross(points[2] - origin).normalize();
			spk::Vector3 v = normal.cross(u);

			float distance = normal.dot(p_point - origin);
			if (std::fabs(distance) > tol3D)
			{
				return false;
			}

			auto project = [&](const spk::Vector3 &P)
			{
				spk::Vector3 rel = P - origin;
				return spk::Vector2(rel.dot(u), rel.dot(v));
			};

			std::vector<spk::Vector2> poly2d;
			poly2d.reserve(points.size());
			for (const spk::Vector3 &pt : points)
			{
				poly2d.push_back(project(pt));
			}

			const spk::Vector2 p = project(p_point);

			for (size_t i = 0, j = poly2d.size() - 1; i < poly2d.size(); j = i++)
			{
				if (_pointOnSegment2D(p, poly2d[j], poly2d[i], tol2D))
				{
					return true;
				}
			}

			// Tolerant ray casting (avoid vertex double-count with slight y-nudge)
			bool inside = false;
			const float py = p.y - tol2D;

			for (size_t i = 0, j = poly2d.size() - 1; i < poly2d.size(); j = i++)
			{
				const spk::Vector2 &A = poly2d[j];
				const spk::Vector2 &B = poly2d[i];

				const bool aboveA = (A.y > py);
				const bool aboveB = (B.y > py);

				if (aboveA != aboveB)
				{
					float xIntersect = (B.x - A.x) * (py - A.y) / (B.y - A.y) + A.x;

					if (std::fabs(xIntersect - p.x) <= tol2D)
					{
						return true;
					}
					if (p.x < xIntersect)
					{
						inside = !inside;
					}
				}
			}

			return inside;
		}

		bool contains(const Polygon &p_polygon) const
		{
			if (isCoplanar(p_polygon) == false)
			{
				return false;
			}

			// For full containment, every vertex of the inner polygon must be inside or on the boundary.
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
	};
}
