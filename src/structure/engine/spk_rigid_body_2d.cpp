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

	void RigidBody2D::start()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		_rigidBodies.push_back(this);
	}

	void RigidBody2D::stop()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		auto it = std::remove_if(
			_rigidBodies.begin(), _rigidBodies.end(), [this](const spk::SafePointer<RigidBody2D> &p_ptr) { return (p_ptr.get() == this); });
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

	spk::BoundingBox2D RigidBody2D::_computeLocalBoundingBox() const
	{
		spk::BoundingBox2D result{};
		bool initialized = false;
		const auto &collider = _collider;
		if (collider != nullptr)
		{
			for (const auto &unit : collider->units())
			{
				for (const auto &vertex : unit.points())
				{
					if (initialized == false)
					{
						result.min = vertex;
						result.max = vertex;
						initialized = true;
					}
					else
					{
						result.min = spk::Vector2::min(result.min, vertex);
						result.max = spk::Vector2::max(result.max, vertex);
					}
				}
			}
		}
		return (result);
	}

	spk::BoundingBox2D RigidBody2D::boundingBox() const
	{
		spk::BoundingBox2D box = _computeLocalBoundingBox();
		if (owner() != nullptr)
		{
			const spk::Vector3 &pos = owner()->transform().position();
			box.min += pos.xy();
			box.max += pos.xy();
		}
		return (box);
	}

	std::vector<spk::SafePointer<RigidBody2D>> RigidBody2D::getRigidBodies()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		return (_rigidBodies);
	}

	namespace
	{
		spk::BoundingBox2D computeBoundingBox(const RigidBody2D *p_body, const spk::Matrix4x4 &p_model)
		{
			spk::BoundingBox2D result{};
			bool initialized = false;
			const auto &collider = p_body->collider();
			if (collider != nullptr)
			{
				for (const auto &unit : collider->units())
				{
					for (const auto &vertex : unit.points())
					{
						spk::Vector3 transformed = p_model * spk::Vector3(vertex.x, vertex.y, 0.0f);
						spk::Vector2 xy = transformed.xy();
						if (initialized == false)
						{
							result.min = xy;
							result.max = xy;
							initialized = true;
						}
						else
						{
							result.min = spk::Vector2::min(result.min, xy);
							result.max = spk::Vector2::max(result.max, xy);
						}
					}
				}
			}
			return (result);
		}

		bool boxesIntersect(const spk::BoundingBox2D &p_a, const spk::BoundingBox2D &p_b)
		{
			return ((p_a.min.x <= p_b.max.x && p_a.max.x >= p_b.min.x && p_a.min.y <= p_b.max.y && p_a.max.y >= p_b.min.y) == true);
		}
	}

	bool RigidBody2D::intersect(
		const RigidBody2D *p_a, const spk::Matrix4x4 &p_transformA, const RigidBody2D *p_b, const spk::Matrix4x4 &p_transformB)
	{
		spk::BoundingBox2D boxA = computeBoundingBox(p_a, p_transformA);
		spk::BoundingBox2D boxB = computeBoundingBox(p_b, p_transformB);
		return (boxesIntersect(boxA, boxB));
	}
}
