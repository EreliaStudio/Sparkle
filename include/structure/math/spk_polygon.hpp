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
			spk::cout << L"[Polygon] Coplanarity check" << std::endl;
			spk::cout << L"  First polygon points:" << std::endl;
			for (const spk::Vector3 &pt : points)
			{
				spk::cout << L"    " << pt << std::endl;
			}
			spk::cout << L"  Second polygon points:" << std::endl;
			for (const spk::Vector3 &pt : p_polygon.points)
			{
				spk::cout << L"    " << pt << std::endl;
			}

			if (isPlanar() == false || p_polygon.isPlanar() == false)
			{
				spk::cout << L"  At least one polygon is not planar" << std::endl;
				return false;
			}

			spk::Vector3 n = normal();
			spk::Vector3 otherNormal = p_polygon.normal();
			spk::cout << L"  Normal A: " << n << L" Normal B: " << otherNormal << std::endl;

			bool sameNormal = n == otherNormal;
			bool oppositeNormal = n == -otherNormal;
			spk::cout << L"  Same normal: " << sameNormal << L" Opposite normal: " << oppositeNormal << std::endl;

			if (sameNormal == false && oppositeNormal == false)
			{
				spk::cout << L"  Normals are neither identical nor opposite" << std::endl;
				return false;
			}

			const spk::Vector3 &origin = points[0];
			float distance = n.dot(p_polygon.points[0] - origin);
			spk::cout << L"  Distance between planes: " << distance << std::endl;
			bool result = std::abs(distance) <= std::numeric_limits<float>::epsilon();
			spk::cout << L"  Coplanar result: " << result << std::endl;
			return result;
		}

		bool contains(const spk::Vector3 &p_point) const
		{
			spk::cout << L"[Polygon] Point containment check" << std::endl;
			spk::cout << L"  Polygon points:" << std::endl;
			for (const spk::Vector3 &pt : points)
			{
				spk::cout << L"    " << pt << std::endl;
			}
			spk::cout << L"  Point: " << p_point << std::endl;

			if (points.size() < 3)
			{
				spk::cout << L"  Polygon has fewer than 3 points" << std::endl;
				return false;
			}
			const spk::Vector3 &origin = points[0];
			spk::Vector3 u = (points[1] - origin).normalize();
			spk::Vector3 normal = (points[1] - origin).cross(points[2] - origin).normalize();
			spk::Vector3 v = normal.cross(u);

			spk::cout << L"  Origin: " << origin << L" u: " << u << L" v: " << v << L" normal: " << normal << std::endl;

			float distance = normal.dot(p_point - origin);
			spk::cout << L"  Distance from plane: " << distance << std::endl;
			if (std::abs(distance) > std::numeric_limits<float>::epsilon())
			{
				spk::cout << L"  Point is not in the polygon plane" << std::endl;
				return false;
			}

			auto project = [&](const spk::Vector3 &p_point3D)
			{
				spk::Vector3 rel = p_point3D - origin;
				return spk::Vector2(rel.dot(u), rel.dot(v));
			};

			std::vector<spk::Vector2> poly2d;
			poly2d.reserve(points.size());
			for (const spk::Vector3 &pt : points)
			{
				poly2d.push_back(project(pt));
			}

			spk::Vector2 p = project(p_point);
			spk::cout << L"  Projected point: " << p << std::endl;

			for (size_t i = 0, j = poly2d.size() - 1; i < poly2d.size(); j = i++)
			{
				const spk::Vector2 &pi = poly2d[i];
				const spk::Vector2 &pj = poly2d[j];
				float cross = (pj.x - pi.x) * (p.y - pi.y) - (pj.y - pi.y) * (p.x - pi.x);
				if (std::abs(cross) <= std::numeric_limits<float>::epsilon() && p.x >= std::min(pi.x, pj.x) && p.x <= std::max(pi.x, pj.x) &&
					p.y >= std::min(pi.y, pj.y) && p.y <= std::max(pi.y, pj.y))
				{
					spk::cout << L"  Point lies on an edge" << std::endl;
					return true;
				}
			}

			bool result = false;
			for (size_t i = 0, j = poly2d.size() - 1; i < poly2d.size(); j = i++)
			{
				const spk::Vector2 &pi = poly2d[i];
				const spk::Vector2 &pj = poly2d[j];
				if (((pi.y > p.y) != (pj.y > p.y)) == true)
				{
					float intersectX = (pj.x - pi.x) * (p.y - pi.y) / (pj.y - pi.y) + pi.x;
					if (std::abs(intersectX - p.x) <= std::numeric_limits<float>::epsilon())
					{
						spk::cout << L"  Point lies on horizontal edge" << std::endl;
						return true;
					}
					if (p.x < intersectX)
					{
						result = (result == false);
					}
				}
				else
				{
					float cross = (p.x - pi.x) * (pj.y - pi.y) - (p.y - pi.y) * (pj.x - pi.x);
					if (std::abs(cross) <= std::numeric_limits<float>::epsilon() && p.x >= std::min(pi.x, pj.x) && p.x <= std::max(pi.x, pj.x) &&
						p.y >= std::min(pi.y, pj.y) && p.y <= std::max(pi.y, pj.y))
					{
						spk::cout << L"  Point lies on segment" << std::endl;
						return true;
					}
				}
			}
			spk::cout << L"  Containment result: " << result << std::endl;
			return result;
		}

		bool contains(const Polygon &p_polygon) const
		{
			spk::cout << L"[Polygon] Polygon containment check" << std::endl;
			spk::cout << L"  Outer polygon points:" << std::endl;
			for (const spk::Vector3 &pt : points)
			{
				spk::cout << L"    " << pt << std::endl;
			}
			spk::cout << L"  Inner polygon points:" << std::endl;
			for (const spk::Vector3 &pt : p_polygon.points)
			{
				spk::cout << L"    " << pt << std::endl;
			}

			if (isCoplanar(p_polygon) == false)
			{
				spk::cout << L"  Polygons are not coplanar" << std::endl;
				return false;
			}
			for (const spk::Vector3 &pt : p_polygon.points)
			{
				bool inside = contains(pt);
				spk::cout << L"  Point " << pt << L" inside: " << inside << std::endl;
				if (inside == false)
				{
					spk::cout << L"  Polygon is not contained" << std::endl;
					return false;
				}
			}
			spk::cout << L"  Polygon is fully contained" << std::endl;
			return true;
		}
	};
}
