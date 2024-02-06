#pragma once

#include <string>
#include "design_pattern/spk_activable_object.hpp"

namespace spk
{
	class GameObject;

	class GameComponent : public spk::ActivateObject
	{
		friend class GameObject;

	private:
		static inline GameObject* _creatingObject = nullptr;
		GameObject* _owner;
		std::string _name;

	public:
		GameComponent(const std::string& p_name);

		GameObject* owner();
		const GameObject* owner() const;
		std::string fullName() const;
		const std::string& name() const;

		virtual void onRender() = 0;
		virtual void onUpdate() = 0;
	};
}