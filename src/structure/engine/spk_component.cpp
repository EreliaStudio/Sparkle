#include "structure/engine/spk_component.hpp"

#include "structure/engine/spk_entity.hpp"

namespace spk
{
	Component::Component(const std::wstring& p_name) :
		_name(p_name),
		_owner(nullptr)
	{

	}

	void Component::setName(const std::wstring& p_name)
	{
		_name = p_name;
	}

	void Component::setPriority(const int& p_priority)
	{
		_priority = p_priority;
		if (_owner != nullptr)
		{
			_owner->sortComponent();
		}
	}

	const std::wstring& Component::name() const
	{
		return (_name);
	}

	int Component::priority() const
	{
		return (_priority);
	}

	spk::SafePointer<Entity> Component::owner()
	{
		return (_owner);
	}

	const spk::SafePointer<const Entity> Component::owner() const
	{
		return (_owner);
	}

	void Component::start()
	{

	}

	void Component::awake()
	{

	}

	void Component::sleep()
	{

	}

	void Component::stop()
	{

	}

	void Component::render()
	{

	};

	void Component::update(const long long& p_deltaTime)
	{

	};
}