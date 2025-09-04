#include "structure/engine/spk_rigid_body_2d.hpp"
#include "structure/engine/spk_entity.hpp"
#include <algorithm>

namespace spk
{
	RigidBody2D::RigidBody2D(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	RigidBody2D::~RigidBody2D()
	{
		stop();
	}

	void RigidBody2D::awake()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		_rigidBodies.push_back(this);
	}

	void RigidBody2D::sleep()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		auto it = std::remove_if(
			_rigidBodies.begin(), _rigidBodies.end(), [this](const spk::SafePointer<const RigidBody2D> &p_ptr) { return p_ptr.get() == this; });
		_rigidBodies.erase(it, _rigidBodies.end());
	}

	void RigidBody2D::setCollider(const spk::SafePointer<const spk::CollisionMesh2D> &p_collider)
	{
		_collider = p_collider;
	}

	const spk::SafePointer<const spk::CollisionMesh2D> &RigidBody2D::collider() const
	{
		return (_collider);
	}

	const std::vector<spk::SafePointer<RigidBody2D>>& RigidBody2D::getRigidBodies()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		return (_rigidBodies);
	}

	namespace
	{
		inline bool segmentsIntersect(const spk::Edge2D &p_ab, const spk::Edge2D &p_cd)
		{
			const float eps = spk::Constants::pointPrecision;

			const float o1 = p_ab.orientation(p_cd.first());
			const float o2 = p_ab.orientation(p_cd.second());
			const float o3 = p_cd.orientation(p_ab.first());
			const float o4 = p_cd.orientation(p_ab.second());

			// Proper intersection (straddling)
			const bool straddleAB = ((o1 > eps && o2 < -eps) || (o1 < -eps && o2 > eps));
			const bool straddleCD = ((o3 > eps && o4 < -eps) || (o3 < -eps && o4 > eps));
			if (straddleAB && straddleCD)
			{
				return true;
			}

			if (std::fabs(o1) <= eps && p_ab.contains(p_cd.first(), true))
			{
				return true;
			}
			if (std::fabs(o2) <= eps && p_ab.contains(p_cd.second(), true))
			{
				return true;
			}
			if (std::fabs(o3) <= eps && p_cd.contains(p_ab.first(), true))
			{
				return true;
			}
			if (std::fabs(o4) <= eps && p_cd.contains(p_ab.second(), true))
			{
				return true;
			}

			return false;
		}

		bool pointInPolygon(const spk::Vector2 &p_p, const std::vector<spk::Vector2> &p_poly)
		{
			const size_t n = p_poly.size();
			if (n < 3)
			{
				return false;
			}

			for (size_t i = 0; i < n; ++i)
			{
				spk::Edge2D e(p_poly[i], p_poly[(i + 1) % n]);
				if (e.contains(p_p, true))
				{
					return true;
				}
			}

			bool inside = false;
			for (size_t i = 0, j = n - 1; i < n; j = i++)
			{
				const spk::Vector2 &A = p_poly[i];
				const spk::Vector2 &B = p_poly[j];

				const bool crosses = ((A.y > p_p.y) != (B.y > p_p.y));
				if (crosses)
				{
					const float t = (p_p.y - A.y) / (B.y - A.y);
					const float xInt = A.x + t * (B.x - A.x);
					if (xInt >= p_p.x - spk::Constants::pointPrecision)
					{
						inside = !inside;
					}
				}
			}
			return inside;
		}

		std::vector<std::vector<spk::Vector2>> collectPolygons2D(
			const spk::SafePointer<const spk::CollisionMesh2D> &p_collider, const spk::Transform &p_transform)
		{
			std::vector<std::vector<spk::Vector2>> result;
			if (p_collider == nullptr)
			{
				return result;
			}

			const spk::Vector2 offset = p_transform.position().xy();

			for (const auto &unit : p_collider->units())
			{
				const auto &pts = unit.points();
				if (pts.empty())
				{
					continue;
				}

				std::vector<spk::Vector2> poly;
				poly.reserve(pts.size());
				for (const auto &p : pts)
				{
					poly.emplace_back(p + offset);
				}
				result.emplace_back(std::move(poly));
			}
			return result;
		}

		bool polygonsIntersect2D(const std::vector<spk::Vector2> &p_a, const std::vector<spk::Vector2> &p_b)
		{
			if (p_a.size() < 2 || p_b.size() < 2)
			{
				return false;
			}

			for (size_t i = 0; i < p_a.size(); ++i)
			{
				spk::Edge2D eA(p_a[i], p_a[(i + 1) % p_a.size()]);
				for (size_t j = 0; j < p_b.size(); ++j)
				{
					spk::Edge2D eB(p_b[j], p_b[(j + 1) % p_b.size()]);
					if (segmentsIntersect(eA, eB))
					{
						return true;
					}
				}
			}

			if (p_a.empty() == false && pointInPolygon(p_a[0], p_b) == true)
			{
				return true;
			}
			if (p_b.empty() == false && pointInPolygon(p_b[0], p_a) == true)
			{
				return true;
			}

			return false;
		}
	}

	bool RigidBody2D::intersect(const spk::SafePointer<RigidBody2D> p_other) const
	{
		const BoundingBox2D boxA = collider()->boundingBox().place(owner()->transform().position().xy());
		const BoundingBox2D boxB = p_other->collider()->boundingBox().place(p_other->owner()->transform().position().xy());
		if (boxA.intersect(boxB) == false)
		{
			return false;
		}

		const auto polysA = collectPolygons2D(collider(), owner()->transform());
		const auto polysB = collectPolygons2D(p_other->collider(), p_other->owner()->transform());

		for (const auto &pa : polysA)
		{
			for (const auto &pb : polysB)
			{
				if (polygonsIntersect2D(pa, pb) == true)
				{
					return true;
				}
			}
		}
		return false;
	}
}
