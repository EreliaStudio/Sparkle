#include "structure/engine/3d/spk_collider.hpp"
#include "structure/engine/3d/spk_entity.hpp"
#include <algorithm>
#include <vector>

namespace spk
{
	Collider::Collider(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	Collider::~Collider()
	{
		stop();
	}

	std::vector<spk::SafePointer<const Collider>> Collider::_executeCollisionTest()
	{
		std::vector<spk::SafePointer<const Collider>> result;

		std::lock_guard<std::mutex> lock(_collidersMutex);
		for (const auto &body : _colliders)
		{
			if (body.get() == this)
			{
				continue;
			}
			if (intersect(body) == true)
			{
				result.push_back(body);
				_onCollisionEnterProvider.trigger(body->owner());
			}
		}
		return (result);
	}

	void Collider::start()
	{
		_onOwnerOnTransformEditionContract = owner().upCast<spk::Entity>()->transform().addOnEditionCallback(
			[&]()
			{
				_updateCollisionCache();
				std::vector<spk::SafePointer<const Collider>> collidedCollider = _executeCollisionTest();
			});
	}

	void Collider::awake()
	{
		{
			std::lock_guard<std::mutex> lock(_collidersMutex);
			_colliders.push_back(this);
		}
		_updateCollisionCache();
	}

	void Collider::sleep()
	{
		std::lock_guard<std::mutex> lock(_collidersMutex);
		auto it =
			std::remove_if(_colliders.begin(), _colliders.end(), [this](const spk::SafePointer<Collider> &p_ptr) { return (p_ptr.get() == this); });
		_colliders.erase(it, _colliders.end());
	}

	void Collider::setCollisionMesh(const spk::SafePointer<const spk::CollisionMesh> &p_collisionMesh)
	{
		_collisionMesh = p_collisionMesh;
		_updateCollisionCache();
	}

	const spk::SafePointer<const spk::CollisionMesh> &Collider::collisionMesh() const
	{
		return (_collisionMesh);
	}

	const std::vector<spk::SafePointer<Collider>> &Collider::getColliders()
	{
		std::lock_guard<std::mutex> lock(_collidersMutex);
		return (_colliders);
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

	void Collider::_updateCollisionCache()
	{
		if (_collisionMesh != nullptr)
		{
			_boundingBox = _collisionMesh->boundingBox().place(owner().upCast<spk::Entity>()->transform().position());
			_polygons = collectPolygons(_collisionMesh, owner().upCast<spk::Entity>()->transform());
		}
		else
		{
			_boundingBox = spk::BoundingBox();
			_polygons.clear();
		}
	}

	bool Collider::intersect(const spk::SafePointer<Collider> p_other) const
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

	void Collider::setMode(const Collider::Mode &p_mode)
	{
		_mode = p_mode;
	}

	const Collider::Mode &Collider::mode() const
	{
		return (_mode);
	}

	Collider::CollisionEnterContract Collider::onCollisionEnter(const CollisionEnterJob &p_job)
	{
		return (_onCollisionEnterProvider.subscribe(p_job));
	}
}
