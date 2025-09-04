#include "structure/math/spk_polygon_2d.hpp"

namespace spk
{
		Polygon2D::Polygon2D(const std::vector<spk::Vector2> &p_points) :
			_points(p_points)
		{
			for (const auto &p : _points)
			{
				_boundingBox.addPoint(p);
			}
		}

		const std::vector<spk::Vector2> &Polygon2D::points() const
		{
			return _points;
		}

		const BoundingBox2D &Polygon2D::boundingBox() const
		{
			return _boundingBox;
		}

		Polygon2D Polygon2D::fromLoop(const std::vector<spk::Vector2> &p_points)
		{
			return Polygon2D(p_points);
		}

		bool Polygon2D::intersect(const spk::Polygon2D& p_other) const
		{
			if (_points.size() < 3 || p_other.points().size() < 3)
			{
				return false;
			}

			if (_boundingBox.intersect(p_other.boundingBox()) == false)
			{
				return false;
			}

			const float eps = spk::Constants::pointPrecision;

			auto buildEdges = [](const std::vector<spk::Vector2>& pts)
			{
				std::vector<spk::Edge2D> edges;
				edges.reserve(pts.size());
				for (size_t i = 0; i < pts.size(); ++i)
				{
					edges.emplace_back(pts[i], pts[(i + 1) % pts.size()]);
				}
				return edges;
			};

			const auto& A = _points;
			const auto& B = p_other.points();

			auto edgesA = buildEdges(A);
			auto edgesB = buildEdges(B);

			auto segIntersect = [&](const spk::Edge2D& e1, const spk::Edge2D& e2) -> bool
			{
				const spk::Vector2 a1 = e1.first();
				const spk::Vector2 a2 = e1.second();
				const spk::Vector2 b1 = e2.first();
				const spk::Vector2 b2 = e2.second();

				const float o1 = e1.orientation(b1);
				const float o2 = e1.orientation(b2);
				const float o3 = e2.orientation(a1);
				const float o4 = e2.orientation(a2);

				auto isZero = [&](float v) { return std::fabs(v) <= eps; };

				if ((o1 * o2) < -eps && (o3 * o4) < -eps)
				{
					return true;
				}

				auto onSegment = [&](const spk::Edge2D& e, const spk::Vector2& p) -> bool
				{
					return e.contains(p, true) == true;
				};

				if (isZero(o1) && onSegment(e1, b1)) return true;
				if (isZero(o2) && onSegment(e1, b2)) return true;
				if (isZero(o3) && onSegment(e2, a1)) return true;
				if (isZero(o4) && onSegment(e2, a2)) return true;

				if (e1.isColinear(e2) == true)
				{
					const float t1 = e1.project(b1);
					const float t2 = e1.project(b2);
					const float len = e1.delta().norm();

					const float lo = std::min(t1, t2);
					const float hi = std::max(t1, t2);

					const float overlapLo = std::max(0.0f, lo);
					const float overlapHi = std::min(len, hi);

					if (overlapHi >= overlapLo - eps)
					{
						return true;
					}
				}

				return false;
			};

			for (const auto& ea : edgesA)
			{
				for (const auto& eb : edgesB)
				{
					if (segIntersect(ea, eb) == true)
					{
						return true;
					}
				}
			}

			auto pointInPolygon = [&](const spk::Vector2& p, const std::vector<spk::Vector2>& poly) -> bool
			{
				for (size_t i = 0; i < poly.size(); ++i)
				{
					spk::Edge2D e(poly[i], poly[(i + 1) % poly.size()]);
					if (e.contains(p, true) == true)
						return true;
				}

				bool inside = false;
				for (size_t i = 0, k = poly.size() - 1; i < poly.size(); k = i++)
				{
					const spk::Vector2& vi = poly[i];
					const spk::Vector2& vk = poly[k];

					const bool condY = ((vi.y > p.y) != (vk.y > p.y));
					if (condY)
					{
						const float xCross = vk.x + (vi.x - vk.x) * ((p.y - vk.y) / (vi.y - vk.y));
						if (xCross >= p.x - eps)
						{
							inside = !inside;
						}
					}
				}
				return inside;
			};

			if (pointInPolygon(A.front(), B) == true) return true;
			if (pointInPolygon(B.front(), A) == true) return true;

			return false;
		}
}
