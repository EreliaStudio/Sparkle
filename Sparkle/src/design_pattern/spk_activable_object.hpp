#pragma once

namespace spk
{
	/**
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
	private:
		bool _isActive = false;

	public:
		void switchActivationState()
		{
			_isActive = !_isActive;
		}

		void activate()
		{
			_isActive = true;
		}

		void deactivate()
		{
			_isActive = false;
		}

		bool isActive() const
		{
			return (_isActive);
		}
	};
}