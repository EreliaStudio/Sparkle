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
		std::wstring _name;
		spk::SafePointer<Entity> _owner;

	public:
		Component(const std::wstring& p_name, spk::SafePointer<Entity> p_owner) :
			_name(p_name),
			_owner(p_owner)
		{

		}

		void setName(const std::wstring& p_name)
		{
			_name = p_name;
		}

		const std::wstring& name() const
		{
			return (_name);
		}

		spk::SafePointer<Entity> owner()
		{
			return (_owner);
		}

		const spk::SafePointer<const Entity> owner() const
		{
			return (_owner);
		}

		virtual void start() {}
		virtual void awake() {}
		virtual void sleep() {}
		virtual void stop() {}

		virtual void render() {};
		virtual void update(const long long& p_deltaTime) {}; // Delta time in millisecond
	};
}