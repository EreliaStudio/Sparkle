#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include <cmath>
#include <vector>

namespace spk
{
	struct Polygon
	{
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
			const spk::Vector3 &origin = points[0];
			spk::Vector3 tmpNormal = normal();
			for (size_t i = 3; i < points.size(); ++i)
			{
				if (std::abs(tmpNormal.dot(points[i] - origin)) > std::numeric_limits<float>::epsilon())
				{
					return false;
				}
			}
			return true;
		}

		bool isCoplanar(const Polygon &p_polygon) const
		{
			return (
				isPlanar() == true && p_polygon.isPlanar() == true && 
				normal() == p_polygon.normal()
			);
		}

		bool contains(const Polygon &p_polygon) const
		{
			if (isCoplanar(p_polygon) == false)
			{
				return false;
			}
			if (points.size() < 3)
			{
				return false;
			}
			const spk::Vector3 &origin = points[0];
			spk::Vector3 u = (points[1] - origin).normalize();
			spk::Vector3 normal = (points[1] - origin).cross(points[2] - origin).normalize();
			spk::Vector3 v = normal.cross(u);

			auto project = [&](const spk::Vector3 &pt)
			{
				spk::Vector3 rel = pt - origin;
				return spk::Vector2(rel.dot(u), rel.dot(v));
			};

			std::vector<spk::Vector2> poly2d;
			poly2d.reserve(points.size());
			for (const spk::Vector3 &pt : points)
			{
				poly2d.push_back(project(pt));
			}

			auto inside = [&](const spk::Vector2 &p)
			{
				bool result = false;
				for (size_t i = 0, j = poly2d.size() - 1; i < poly2d.size(); j = i++)
				{
					const spk::Vector2 &pi = poly2d[i];
					const spk::Vector2 &pj = poly2d[j];
					if (((pi.y > p.y) != (pj.y > p.y)) && (p.x < (pj.x - pi.x) * (p.y - pi.y) / (pj.y - pi.y) + pi.x))
					{
						result = (result == false);
					}
				}
				return result;
			};

			for (const spk::Vector3 &pt : p_polygon.points)
			{
				if (inside(project(pt)) == false)
				{
					return false;
				}
			}
			return true;
		}
	};
}