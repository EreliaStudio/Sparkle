#include "structure/engine/spk_collision_trigger_2d.hpp"
#include "structure/engine/spk_entity.hpp"
#include <algorithm>

namespace spk
{
	CollisionTrigger2D::CollisionTrigger2D(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	void CollisionTrigger2D::setTrigger(const bool &p_state)
	{
		_isTrigger = p_state;
	}

	bool CollisionTrigger2D::isTrigger() const
	{
		return (_isTrigger);
	}

	CollisionTrigger2D::TriggerEnterContract CollisionTrigger2D::onTriggerEnter(const TriggerEnterJob &p_job)
	{
		return (_onTriggerEnterProvider.subscribe(p_job));
	}

	void CollisionTrigger2D::start()
	{
		std::lock_guard<std::mutex> lock(_triggerMutex);
		_triggers.push_back(this);
	}

	void CollisionTrigger2D::stop()
	{
		std::lock_guard<std::mutex> lock(_triggerMutex);
		auto it = std::remove_if(
			_triggers.begin(), _triggers.end(), [this](const spk::SafePointer<CollisionTrigger2D> &p_ptr) { return (p_ptr.get() == this); });
		_triggers.erase(it, _triggers.end());
	}

	void CollisionTrigger2D::startFrame()
	{
		_frameCollisions.clear();
	}

	void CollisionTrigger2D::registerCollision(RigidBody2D *p_body)
	{
		if (_isTrigger == false)
		{
			return;
		}
		_frameCollisions.insert(p_body);
		if (_currentCollisions.contains(p_body) == false)
		{
			_onTriggerEnterProvider.trigger(p_body->owner());
		}
	}

	void CollisionTrigger2D::endFrame()
	{
		_currentCollisions = std::move(_frameCollisions);
	}

	std::vector<spk::SafePointer<CollisionTrigger2D>> CollisionTrigger2D::getTriggers()
	{
		std::lock_guard<std::mutex> lock(_triggerMutex);
		return (_triggers);
	}
}
