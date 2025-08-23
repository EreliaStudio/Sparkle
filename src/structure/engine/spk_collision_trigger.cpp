#include "structure/engine/spk_collision_trigger.hpp"
#include "structure/engine/spk_entity.hpp"
#include <algorithm>

namespace spk
{
	CollisionTrigger::CollisionTrigger(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	void CollisionTrigger::setTrigger(const bool &p_state)
	{
		_isTrigger = p_state;
	}

	bool CollisionTrigger::isTrigger() const
	{
		return (_isTrigger);
	}

	CollisionTrigger::TriggerEnterContract CollisionTrigger::onTriggerEnter(const TriggerEnterJob &p_job)
	{
		return (_onTriggerEnterProvider.subscribe(p_job));
	}

	void CollisionTrigger::start()
	{
		std::lock_guard<std::mutex> lock(_triggerMutex);
		_triggers.push_back(this);
	}

	void CollisionTrigger::stop()
	{
		std::lock_guard<std::mutex> lock(_triggerMutex);
		auto it = std::remove_if(
			_triggers.begin(), _triggers.end(), [this](const spk::SafePointer<CollisionTrigger> &p_ptr) { return (p_ptr.get() == this); });
		_triggers.erase(it, _triggers.end());
	}

	void CollisionTrigger::startFrame()
	{
		_frameCollisions.clear();
	}

	void CollisionTrigger::registerCollision(RigidBody *p_body)
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

	void CollisionTrigger::endFrame()
	{
		_currentCollisions = std::move(_frameCollisions);
	}

	std::vector<spk::SafePointer<CollisionTrigger>> CollisionTrigger::getTriggers()
	{
		std::lock_guard<std::mutex> lock(_triggerMutex);
		return (_triggers);
	}
}
