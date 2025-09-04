#include "structure/engine/spk_rigid_body.hpp"
#include "structure/engine/spk_entity.hpp"
#include <algorithm>
#include <vector>

namespace spk
{
	RigidBody::RigidBody(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	RigidBody::~RigidBody()
	{
		stop();
	}

	std::vector<spk::SafePointer<const RigidBody>> RigidBody::_executeCollisionTest()
	{
		std::vector<spk::SafePointer<const RigidBody>> result;

		return (result);
	}

	void RigidBody::start()
	{
		_onOwnerOnTransformEditionContract = owner()->transform().addOnEditionCallback(
			[&]()
			{
				_updateCollisionCache();
				std::vector<spk::SafePointer<const RigidBody>> collidedRigidBody = _executeCollisionTest();
			});
	}

	void RigidBody::awake()
	{
		{
			std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
			_rigidBodies.push_back(this);
		}
		_updateCollisionCache();
	}

	void RigidBody::sleep()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		auto it = std::remove_if(
			_rigidBodies.begin(), _rigidBodies.end(), [this](const spk::SafePointer<RigidBody> &p_ptr) { return (p_ptr.get() == this); });
		_rigidBodies.erase(it, _rigidBodies.end());
	}

	void RigidBody::setCollisionMesh(const spk::SafePointer<const spk::CollisionMesh> &p_collisionMesh)
	{
		_collisionMesh = p_collisionMesh;
		_updateCollisionCache();
	}

	const spk::SafePointer<const spk::CollisionMesh> &RigidBody::collisionMesh() const
	{
		return (_collisionMesh);
	}

	const std::vector<spk::SafePointer<RigidBody>> &RigidBody::getRigidBodies()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		return (_rigidBodies);
	}

	namespace
	{
		std::vector<spk::Polygon> collectPolygons(const spk::SafePointer<const CollisionMesh> &p_collisionMesh, const spk::Transform &p_transform)
		{
			std::vector<spk::Polygon> result;

			if (p_collisionMesh != nullptr)
			{
				for (const auto &unit : p_collisionMesh->units())
				{
					std::vector<spk::Vector3> transformed;
					const auto &pts = unit.points();
					transformed.reserve(pts.size());
					for (const auto &pt : pts)
					{
						transformed.push_back(p_transform.model() * pt);
					}
					result.push_back(spk::Polygon::fromLoop(transformed));
				}
			}
			return result;
		}
	}

	void RigidBody::_updateCollisionCache()
	{
		if (_collisionMesh != nullptr)
		{
			_boundingBox = _collisionMesh->boundingBox().place(owner()->transform().position());
			_polygons = collectPolygons(_collisionMesh, owner()->transform());
		}
		else
		{
			_boundingBox = spk::BoundingBox();
			_polygons.clear();
		}
	}

	bool RigidBody::intersect(const spk::SafePointer<RigidBody> p_other) const
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
