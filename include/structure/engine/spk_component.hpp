#pragma once

#include <string>
#include "structure/spk_safe_pointer.hpp"
#include "structure/system/event/spk_event.hpp"
#include "structure/design_pattern/spk_activable_object.hpp"

namespace spk
{
	class Entity;

	class Component : public spk::ActivableObject
	{
		friend class Entity;

	private:
		int _priority;
		std::wstring _name;
		spk::SafePointer<Entity> _owner;

	public:
		Component(const std::wstring& p_name);

		~Component();

		void setName(const std::wstring& p_name);
		void setPriority(const int& p_priority);

		const std::wstring& name() const;
		int priority() const;
		spk::SafePointer<Entity> owner();
		const spk::SafePointer<const Entity> owner() const;

		virtual void start();
		virtual void awake();
		virtual void sleep();
		virtual void stop();

		virtual void onPaintEvent(spk::PaintEvent& p_event);
		virtual void onUpdateEvent(spk::UpdateEvent& p_event);
		virtual void onKeyboardEvent(spk::KeyboardEvent& p_event);
		virtual void onMouseEvent(spk::MouseEvent& p_event);
		virtual void onControllerEvent(spk::ControllerEvent& p_event);
		virtual void onTimerEvent(spk::TimerEvent& p_event);
	};
}