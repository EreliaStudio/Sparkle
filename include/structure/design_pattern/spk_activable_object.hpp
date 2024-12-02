#pragma once

#include "structure/design_pattern/spk_stateful_object.hpp"

namespace spk
{
	/**
	 * @class ActivableObject
	 * @brief A class that represents an object with an active or inactive state, triggering callbacks on state changes.
	 * 
	 * The `ActivableObject` class inherits from `StatefulObject<bool>` and provides a specialized interface
	 * for managing active and inactive states. It allows the registration of callbacks for activation and deactivation
	 * events and ensures that state transitions trigger the appropriate callbacks.
	 * 
	 * Example usage:
	 * @code
	 * spk::ActivableObject activable;
	 * 
	 * activable.addActivationCallback([]() { std::cout << "Activated!" << std::endl; });
	 * activable.addDeactivationCallback([]() { std::cout << "Deactivated!" << std::endl; });
	 * 
	 * activable.activate();   // Outputs: Activated!
	 * activable.deactivate(); // Outputs: Deactivated!
	 * @endcode
	 */
	class ActivableObject : public StatefulObject<bool>
	{
	public:
		using Contract = StatefulObject::Contract; /**< @brief Type alias for the callback subscription contract. */

	public:
		/**
		 * @brief Constructs an `ActivableObject` with an initial inactive state.
		 */
		ActivableObject();

		/**
		 * @brief Virtual destructor.
		 */
		virtual ~ActivableObject();

		/**
		 * @brief Activates the object, setting its state to `true` and triggering activation callbacks.
		 */
		void activate();

		/**
		 * @brief Deactivates the object, setting its state to `false` and triggering deactivation callbacks.
		 */
		void deactivate();

		/**
		 * @brief Checks if the object is currently active.
		 * 
		 * @return `true` if the object is active, otherwise `false`.
		 */
		bool isActive() const;

		/**
		 * @brief Adds a callback to be triggered when the object is activated.
		 * 
		 * @param p_callback The function to call upon activation.
		 * @return A `Contract` object that can be used to manage the callback subscription.
		 */
		Contract addActivationCallback(const std::function<void()>& p_callback);

		/**
		 * @brief Adds a callback to be triggered when the object is deactivated.
		 * 
		 * @param p_callback The function to call upon deactivation.
		 * @return A `Contract` object that can be used to manage the callback subscription.
		 */
		Contract addDeactivationCallback(const std::function<void()>& p_callback);

	private:
		using StatefulObject<bool>::setState; /**< @brief Hides `setState` from the public interface. */
		using StatefulObject<bool>::addCallback; /**< @brief Hides `addCallback` from the public interface. */
		using StatefulObject<bool>::state; /**< @brief Hides `state` from the public interface. */
	};
}
