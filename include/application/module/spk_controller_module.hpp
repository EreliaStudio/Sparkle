#pragma once

#include "application/module/spk_module.hpp"

#include "structure/system/event/spk_event.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	/**
	 * @class ControllerModule
	 * @brief A module for managing and handling controller events.
	 * 
	 * The `ControllerModule` listens for events related to a controller (joystick, triggers, buttons, etc.),
	 * processes them, and updates the state of a linked widget. This class extends `spk::Module` and uses
	 * `spk::ControllerEvent` as its event type.
	 * 
	 * Example usage:
	 * @code
	 * spk::ControllerModule controllerModule;
	 * spk::Widget* rootWidget = new spk::Widget();
	 * controllerModule.linkToWidget(rootWidget);
	 * 
	 * // Process controller events
	 * controllerModule.treatMessages();
	 * @endcode
	 */
	class ControllerModule : public spk::Module<spk::ControllerEvent>
	{
	private:
		spk::Widget* _rootWidget; /**< @brief Pointer to the root widget that receives controller events. */
		spk::Controller _controller; /**< @brief Stores the current state of the controller. */

		/**
		 * @brief Processes a specific controller event.
		 * 
		 * This method updates the state of the controller based on the event and forwards the event to the root widget.
		 * 
		 * @param p_event The controller event to process.
		 */
		void _treatEvent(spk::ControllerEvent&& p_event) override;

		/**
		 * @brief Converts a generic event into a controller event.
		 * 
		 * @param p_event The generic event to convert.
		 * @return The converted controller event.
		 */
		spk::ControllerEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		/**
		 * @brief Constructs a `ControllerModule` instance.
		 */
		ControllerModule();

		/**
		 * @brief Links the module to a root widget.
		 * 
		 * This method sets the root widget to receive processed controller events.
		 * 
		 * @param p_rootWidget Pointer to the root widget.
		 */
		void linkToWidget(spk::Widget* p_rootWidget);

		/**
		 * @brief Retrieves the current state of the controller.
		 * 
		 * @return A reference to the `spk::Controller` object representing the current state.
		 */
		const spk::Controller& controller() const;
	};
}