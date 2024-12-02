#pragma once

#include "application/module/spk_module.hpp"
#include "structure/system/event/spk_event.hpp"
#include "spk_debug_macro.hpp"

namespace spk
{
	/**
	 * @class MouseModule
	 * @brief A module for managing and handling mouse events.
	 * 
	 * The `MouseModule` listens for events related to the mouse (button presses, motion, wheel scrolling, etc.),
	 * processes them, and updates the state of a linked widget. This class extends `spk::Module` and uses
	 * `spk::MouseEvent` as its event type.
	 * 
	 * Example usage:
	 * @code
	 * spk::MouseModule mouseModule;
	 * spk::Widget* rootWidget = new spk::Widget();
	 * mouseModule.linkToWidget(rootWidget);
	 * 
	 * // Process mouse events
	 * mouseModule.treatMessages();
	 * @endcode
	 */
	class MouseModule : public spk::Module<spk::MouseEvent>
	{
	private:
		spk::Widget* _rootWidget; /**< @brief Pointer to the root widget that receives mouse events. */
		spk::Mouse _mouse; /**< @brief Stores the current state of the mouse. */

		/**
		 * @brief Processes a specific mouse event.
		 * 
		 * This method updates the state of the mouse based on the event and forwards the event to the root widget.
		 * 
		 * @param p_event The mouse event to process.
		 */
		void _treatEvent(spk::MouseEvent&& p_event) override;

		/**
		 * @brief Converts a generic event into a mouse event.
		 * 
		 * @param p_event The generic event to convert.
		 * @return The converted mouse event.
		 */
		spk::MouseEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		/**
		 * @brief Constructs a `MouseModule` instance.
		 */
		MouseModule();

		/**
		 * @brief Links the module to a root widget.
		 * 
		 * This method sets the root widget to receive processed mouse events.
		 * 
		 * @param p_rootWidget Pointer to the root widget.
		 */
		void linkToWidget(spk::Widget* p_rootWidget);

		/**
		 * @brief Retrieves the current state of the mouse.
		 * 
		 * @return A reference to the `spk::Mouse` object representing the current state.
		 */
		const spk::Mouse& mouse() const;
	};
}
