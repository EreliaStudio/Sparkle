#pragma once

#include <string>
#include "design_pattern/spk_activable_object.hpp"

namespace spk
{
	class GameObject;

	/**
	 * @brief Base class for game components that can be attached to GameObjects.
	 * 
	 * This class serves as a base class for various game components that can be attached to GameObjects.
	 * It provides a common interface for handling rendering and updating logic in a game. Derived classes should implement
	 * the `onRender` and `onUpdate` methods to define specific behavior for rendering and updating the component.
	 * 
	 * This class also inherits from `ActivateObject`, allowing components to control their activation state.
	 * Uppon creation, GameComponent are ste as activated.
	 * 
	 * Usage example:
	 * @code
	 * class MyCustomComponent : public spk::GameComponent
	 * {
	 * public:
	 *     MyCustomComponent(const std::string& p_name) : spk::GameComponent(p_name) {}
	 * 
	 *     void onRender() override
	 *     {
	 *         // Implement rendering logic here
	 *     }
	 * 
	 *     void onUpdate() override
	 *     {
	 *         // Implement update logic here
	 *     }
	 * };
	 * @endcode
	 * 
	 * @see GameObject
	 */
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