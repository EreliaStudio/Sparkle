#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"
#include <cmath>
#include <vector>
#include <limits>

#include "spk_debug_macro.hpp"

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
                        return std::abs(distance) <= std::numeric_limits<float>::epsilon();
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

                        auto project = [&](const spk::Vector3 &p_point)
                        {
                                spk::Vector3 rel = p_point - origin;
                                return spk::Vector2(rel.dot(u), rel.dot(v));
                        };

			std::vector<spk::Vector2> poly2d;
			poly2d.reserve(points.size());
			for (const spk::Vector3 &pt : points)
			{
				poly2d.push_back(project(pt));
			}

                        auto onEdge = [&](const spk::Vector2 &p_point)
                        {
                                for (size_t i = 0, j = poly2d.size() - 1; i < poly2d.size(); j = i++)
                                {
                                        const spk::Vector2 &pi = poly2d[i];
                                        const spk::Vector2 &pj = poly2d[j];
                                        float cross = (pj.x - pi.x) * (p_point.y - pi.y) - (pj.y - pi.y) * (p_point.x - pi.x);
                                        if (std::abs(cross) <= std::numeric_limits<float>::epsilon() &&
                                            p_point.x >= std::min(pi.x, pj.x) && p_point.x <= std::max(pi.x, pj.x) &&
                                            p_point.y >= std::min(pi.y, pj.y) && p_point.y <= std::max(pi.y, pj.y))
                                        {
                                                return true;
                                        }
                                }
                                return false;
                        };

                        auto inside = [&](const spk::Vector2 &p_point)
                        {
                                bool result = false;
                                for (size_t i = 0, j = poly2d.size() - 1; i < poly2d.size(); j = i++)
                                {
                                        const spk::Vector2 &pi = poly2d[i];
                                        const spk::Vector2 &pj = poly2d[j];
                                        if (((pi.y > p_point.y) != (pj.y > p_point.y)))
                                        {
                                                float intersectX = (pj.x - pi.x) * (p_point.y - pi.y) / (pj.y - pi.y) + pi.x;
                                                if (std::abs(intersectX - p_point.x) <= std::numeric_limits<float>::epsilon())
                                                {
                                                        return true;
                                                }
                                                if (p_point.x < intersectX)
                                                {
                                                        result = (result == false);
                                                }
                                        }
                                        else
                                        {
                                                float cross = (p_point.x - pi.x) * (pj.y - pi.y) - (p_point.y - pi.y) * (pj.x - pi.x);
                                                if (std::abs(cross) <= std::numeric_limits<float>::epsilon() &&
                                                    p_point.x >= std::min(pi.x, pj.x) && p_point.x <= std::max(pi.x, pj.x) &&
                                                    p_point.y >= std::min(pi.y, pj.y) && p_point.y <= std::max(pi.y, pj.y))
                                                {
                                                        return true;
                                                }
                                        }
                                }
                                return result;
                        };

                        for (const spk::Vector3 &pt : p_polygon.points)
                        {
                                spk::Vector2 projected = project(pt);
                                if (onEdge(projected) == true)
                                {
                                        continue;
                                }
                                if (inside(projected) == false)
                                {
                                        return false;
                                }
                        }
                        return true;
                }
        };
}
