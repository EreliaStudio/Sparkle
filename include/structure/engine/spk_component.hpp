#pragma once

#include <string>
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class Entity;

	class Component
	{
		friend class Entity;

	private:
		int _priority;
		std::wstring _name;
		spk::SafePointer<Entity> _owner;

	public:
		Component(const std::wstring& p_name);

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

		virtual void render();
		virtual void update(const long long& p_deltaTime); // Delta time in millisecond
	};
}