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

	std::vector<spk::SafePointer<RigidBody2D>> RigidBody2D::getRigidBodies()
	{
		std::lock_guard<std::mutex> lock(_rigidBodiesMutex);
		return (_rigidBodies);
	}
}
