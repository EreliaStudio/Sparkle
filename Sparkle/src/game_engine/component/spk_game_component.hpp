#pragma once

#include <string>
#include "design_pattern/spk_activable_object.hpp"

#include "profiler/spk_time_metric.hpp"

namespace spk
{
	class GameObject;

	/**
	 * @class GameComponent
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
		TimeMetric& _timeMetrics;

		virtual void _onRender() = 0;
		virtual void _onUpdate() = 0;

		void update();
		void render();

	public:
		/**
		 * @brief Constructs a GameComponent with a specified name.
		 * 
		 * Initializes the component with the provided name and sets it as activated by default. The component is
		 * initially not associated with any GameObject until it is attached to one.
		 * 
		 * @param p_name The name of the component, used for identification purposes.
		 */
		GameComponent(const std::string& p_name);

		/**
		 * @brief Gets the owner GameObject of this component.
		 * 
		 * Returns a pointer to the GameObject that this component is attached to. If the component is not attached
		 * to any GameObject, returns nullptr.
		 * 
		 * @return A pointer to the owning GameObject, or nullptr if not attached.
		 */
		GameObject* owner();

		/**
		 * @brief Gets the owner GameObject of this component (const version).
		 * 
		 * Returns a const pointer to the GameObject that this component is attached to. This method is const-qualified,
		 * indicating that it does not modify the GameComponent.
		 * 
		 * @return A const pointer to the owning GameObject, or nullptr if not attached.
		 */
		const GameObject* owner() const;

		/**
		 * @brief Gets the full name of the component.
		 * 
		 * Constructs and returns the full name of the component, typically by combining the name of the component
		 * with the name of its owner GameObject. This method can be used to generate unique identifiers for components
		 * within the context of their owning GameObjects.
		 * 
		 * @return A string representing the full name of the component.
		 */
		std::string fullName() const;

		/**
		 * @brief Gets the name of the component.
		 * 
		 * Returns the name of the component as specified during construction. The name is used for identification
		 * purposes within the game engine.
		 * 
		 * @return A const reference to the component's name.
		 */
		const std::string& name() const;
	};
}