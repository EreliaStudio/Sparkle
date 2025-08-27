#include "structure/math/spk_polygon.hpp"

namespace 
{
	float polyScale(const std::vector<spk::Vector3> &p_ps)
	{
		float m = 0.f;
		for (size_t i = 1; i < p_ps.size(); ++i)
		{
			m = std::max(m, (p_ps[i] - p_ps[i - 1]).norm());
		}
		return (m > 0.f ? m : 1.f);
	}

	bool approxEq(float p_a, float p_b, float p_tol)
	{
		return std::fabs(p_a - p_b) <= p_tol;
	}

	bool inRange(float p_x, float p_a, float p_b, float p_tol)
	{
		const float lo = std::min(p_a, p_b) - p_tol;
		const float hi = std::max(p_a, p_b) + p_tol;
		return (p_x >= lo && p_x <= hi);
	}

	bool pointOnSegment2D(const spk::Vector2 &p_p, const spk::Vector2 &p_a, const spk::Vector2 &p_b, float p_tol)
	{
		// Collinearity via cross, then projection within segment via dot
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
}

namespace spk
{
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

	bool Polygon::isPlanar() const
	{
		if (points.size() < 3)
		{
			return false;
		}
		if (points.size() == 3)
		{
			return true;
		}
		const float tol = 1e-6f * polyScale(points);

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

	Plane Polygon::plane() const
	{
		if (points.size() < 3)
		{
			GENERATE_ERROR("Can't generate plane on a polygon with less than 3 points");
		}
		if (isPlanar() == false)
		{
			GENERATE_ERROR("Can't generate plane on a non-planar polygon");
		}

		return (Plane(normal(), points[0]));
	}

	bool Polygon::isCoplanar(const Polygon &p_polygon) const
	{
		if (isPlanar() == false || p_polygon.isPlanar() == false)
		{
			return false;
		}

		return (plane() == p_polygon.plane());
	}

	bool Polygon::contains(const spk::Vector3 &p_point) const
	{
		if (points.size() < 3)
		{
			return false;
		}

		const float scale = polyScale(points);
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

		auto project = [&](const spk::Vector3 &p_p)
		{
			spk::Vector3 rel = p_p - origin;
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
			if (pointOnSegment2D(p, poly2d[j], poly2d[i], tol2D))
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

	bool Polygon::contains(const Polygon &p_polygon) const
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
}
