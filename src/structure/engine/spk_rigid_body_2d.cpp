#include "structure/engine/spk_rigid_body_2d.hpp"
#include "structure/engine/spk_entity.hpp"
#include <algorithm>
#include <vector>

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

	std::vector<spk::SafePointer<const RigidBody2D>> RigidBody2D::_executeCollisionTest()
	{
		std::vector<spk::SafePointer<const RigidBody2D>> result;

		return (result);
	}

	void RigidBody2D::start()
	{
		_onOwnerOnTransformEditionContract = owner()->transform().addOnEditionCallback(
			[&]()
			{
				_updateCollisionCache();
				std::vector<spk::SafePointer<const RigidBody2D>> collidedRigidBody = _executeCollisionTest();
			});
	}

	void RigidBody2D::awake()
	{
		{
			std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
			_rigidBodies.push_back(this);
		}
		_updateCollisionCache();
	}

	void RigidBody2D::sleep()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		auto it = std::remove_if(
			_rigidBodies.begin(), _rigidBodies.end(), [this](const spk::SafePointer<const RigidBody2D> &p_ptr) { return p_ptr.get() == this; });
		_rigidBodies.erase(it, _rigidBodies.end());
	}

	void RigidBody2D::setCollisionMesh(const spk::SafePointer<const spk::CollisionMesh2D> &p_collisionMesh)
	{
		_collisionMesh = p_collisionMesh;
		_updateCollisionCache();
	}

	const spk::SafePointer<const spk::CollisionMesh2D> &RigidBody2D::collisionMesh() const
	{
		return (_collisionMesh);
	}

	const std::vector<spk::SafePointer<RigidBody2D>> &RigidBody2D::getRigidBodies()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		return (_rigidBodies);
	}

	namespace
	{
		std::vector<spk::Polygon2D> collectPolygons2D(
			const spk::SafePointer<const spk::CollisionMesh2D> &p_collisionMesh, const spk::Transform &p_transform)
		{
			std::vector<spk::Polygon2D> result;
			if (p_collisionMesh == nullptr)
			{
				return result;
			}

			const spk::Vector2 offset = p_transform.position().xy();

			for (const auto &unit : p_collisionMesh->units())
			{
				std::vector<spk::Vector2> pts;
				pts.reserve(unit.points().size());
				for (const auto &p : unit.points())
				{
					pts.push_back(p + offset);
				}
				result.push_back(spk::Polygon2D::fromLoop(pts));
			}

			return result;
		}
	}

	void RigidBody2D::_updateCollisionCache()
	{
		if (_collisionMesh != nullptr)
		{
			_boundingBox = _collisionMesh->boundingBox().place(owner()->transform().position().xy());
			_polygons = collectPolygons2D(_collisionMesh, owner()->transform());
		}
		else
		{
			_boundingBox = spk::BoundingBox2D();
			_polygons.clear();
		}
	}

	bool RigidBody2D::intersect(const spk::SafePointer<RigidBody2D> p_other) const
	{
		if (_boundingBox.intersect(p_other->_boundingBox) == false)
		{
			return false;
		}

		for (const auto &pa : _polygons)
		{
			for (const auto &pb : p_other->_polygons)
			{
				if (pa.isSequant(pb) == true)
				{
					return true;
				}
			}
		}
		return false;
	}
}
