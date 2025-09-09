#include "structure/engine/spk_collider_2d.hpp"
#include "structure/engine/spk_entity.hpp"
#include <algorithm>
#include <vector>

namespace spk
{
	Collider2D::Collider2D(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	Collider2D::~Collider2D()
	{
		stop();
	}

	std::vector<spk::SafePointer<const Collider2D>> Collider2D::_executeCollisionTest()
	{
		std::vector<spk::SafePointer<const Collider2D>> result;

		std::lock_guard<std::mutex> lock(_collidersMutex());

		for (const auto &body : _colliders())
		{
			if (body.get() == this)
			{
				continue;
			}

			if (intersect(body) == true)
			{
				result.push_back(body);
				body->_onCollisionEnterProvider.trigger(owner());
			}
		}

		return (result);
	}

	void Collider2D::start()
	{
		_onOwnerOnTransformEditionContract = owner()->transform().addOnEditionCallback(
			[&]()
			{
				_updateCollisionCache();
				std::vector<spk::SafePointer<const Collider2D>> collidedCollider = _executeCollisionTest();
			});
	}

	void Collider2D::awake()
	{
		{
			std::lock_guard<std::mutex> lock(_collidersMutex());
			_colliders().push_back(this);
		}
		_updateCollisionCache();
	}

	void Collider2D::sleep()
	{
		std::lock_guard<std::mutex> lock(_collidersMutex());
		auto it = std::remove_if(
			_colliders().begin(), _colliders().end(), [this](const spk::SafePointer<Collider2D> &p_ptr) { return p_ptr.get() == this; });
		_colliders().erase(it, _colliders().end());
	}

	void Collider2D::setCollisionMesh(const spk::SafePointer<const spk::CollisionMesh2D> &p_collisionMesh)
	{
		_collisionMesh = p_collisionMesh;
		_updateCollisionCache();
	}

	const spk::SafePointer<const spk::CollisionMesh2D> &Collider2D::collisionMesh() const
	{
		return (_collisionMesh);
	}

	const std::vector<spk::SafePointer<Collider2D>> &Collider2D::getColliders()
	{
		std::lock_guard<std::mutex> lock(_collidersMutex());
		return (_colliders());
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

	void Collider2D::_updateCollisionCache()
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

	bool Collider2D::intersect(const spk::SafePointer<Collider2D> p_other) const
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

	void Collider2D::setMode(const Collider2D::Mode &p_mode)
	{
		_mode = p_mode;
	}

	const Collider2D::Mode &Collider2D::mode() const
	{
		return (_mode);
	}

	Collider2D::CollisionEnterContract Collider2D::onCollisionEnter(const CollisionEnterJob &p_job)
	{
		return (_onCollisionEnterProvider.subscribe(p_job));
	}
}
