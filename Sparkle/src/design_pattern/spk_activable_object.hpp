#pragma once

#include <functional>

namespace spk
{
	/**
	 * @class ActivateObject
	 * @brief Manages activation state for objects, intended as a base class.
	 * 
	 * This class provides mechanisms to manage the active state of objects and is designed to be
	 * inherited by other classes that require activation and deactivation capabilities. It includes 
	 * functionalities to activate, deactivate, and toggle the current activation state, making it 
	 * particularly useful for managing resources, components, or features that can be enabled or 
	 * disabled within an application.
	 *
	 * The active state is represented as a boolean value, where `true` indicates that the object is active
	 * and `false` indicates it is inactive. 
	 *
	 * Usage example:
	 * @code
	 * class MyFeature : public spk::ActivateObject
	 * {
	 *     // Implementation specific to MyFeature
	 * };
	 * 
	 * MyFeature feature;
	 * feature.activate(); // Sets the feature as active
	 * if (feature.isActive())
	 * {
	 *     // Perform actions for active state
	 * }
	 * feature.deactivate(); // Sets the feature as inactive
	 * @endcode
	 *
	 * This class does not enforce any specific behavior based on the activation state; it merely provides
	 * a mechanism for tracking and changing this state. Implementing classes or systems can use this state
	 * to determine appropriate actions or behaviors. It is fully intended to be inherited by another class
	 * to extend or customize the activation behavior.
	 */
	class ActivateObject
	{
	public:
		using Callback = std::function<void()>; //!< The callback type used for activation and deactivation

	private:
		bool _isActive = false;
		Callback _activationCallback = nullptr;
		Callback _deactivationCallback = nullptr;

	public:
		/**
		 * @brief Toggles the activation state of the object.
		 * 
		 * If the object is currently active, it becomes inactive, and vice versa.
		 */
		void switchActivationState()
		{
			_isActive = !_isActive;
		}

		/**
		 * @brief Activates the object.
		 * 
		 * Sets the object's activation state to active.
		 */
		void activate()
		{
			if (_isActive == true)
				return ;

			_isActive = true;
			if (_activationCallback != nullptr)
				_activationCallback();
		}

		/**
		 * @brief Deactivates the object.
		 * 
		 * Sets the object's activation state to inactive.
		 */
		void deactivate()
		{
			if (_isActive == false)
				return ;

			_isActive = false;
			if (_deactivationCallback != nullptr)
				_deactivationCallback();
		}

		/**
		 * @brief Checks if the object is active.
		 * 
		 * @return True if the object is active, false otherwise.
		 */
		bool isActive() const
		{
			return (_isActive);
		}

		/**
		 * @brief Define the callback to call when activating the object
		 * @param p_callback The callback to link
		*/
		void setActivationCallback(const Callback& p_callback)
		{
			_activationCallback = p_callback;
		}

		/**
		 * @brief Define the callback to call when deactivating the object
		 * @param p_callback The callback to link
		*/
		void setDeactivationCallback(const Callback& p_callback)
		{
			_deactivationCallback = p_callback;
		}
	};
}