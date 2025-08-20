#include "structure/engine/spk_component.hpp"

#include "structure/engine/spk_entity.hpp"

namespace spk
{
	Component::Component(const std::wstring &p_name) :
		_name(p_name),
		_owner(nullptr)
	{
		addActivationCallback([&]() { awake(); }).relinquish();
		addDeactivationCallback([&]() { sleep(); }).relinquish();
	}

	Component::~Component()
	{
	}

	void Component::setName(const std::wstring &p_name)
	{
		_name = p_name;
	}

	void Component::setPriority(const int &p_priority)
	{
		_priority = p_priority;
		if (_owner != nullptr)
		{
			_owner->sortComponent();
		}
	}

	const std::wstring &Component::name() const
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

	void Component::onGeometryChange(const spk::Geometry2D &p_geometry)
	{
	}

	void Component::onPaintEvent(spk::PaintEvent &p_event)
	{
	}

	void Component::onUpdateEvent(spk::UpdateEvent &p_event)
	{
	}

	void Component::onKeyboardEvent(spk::KeyboardEvent &p_event)
	{
	}

	void Component::onMouseEvent(spk::MouseEvent &p_event)
	{
	}

	void Component::onControllerEvent(spk::ControllerEvent &p_event)
	{
	}

	void Component::onTimerEvent(spk::TimerEvent &p_event)
	{
	}
}