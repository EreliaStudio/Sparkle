#pragma once

#include "application/module/spk_module.hpp"
#include "structure/system/event/spk_event.hpp"
#include "spk_debug_macro.hpp"

namespace spk
{
	/**
	 * @class UpdateModule
	 * @brief A module for managing update events and propagating them to widgets and input devices.
	 * 
	 * The `UpdateModule` listens for periodic update events, enriches them with input states
	 * (mouse, keyboard, and controller), and forwards them to the linked root widget for processing.
	 * This class extends `spk::Module` and uses `spk::UpdateEvent` as its event type.
	 * 
	 * Example usage:
	 * @code
	 * spk::UpdateModule updateModule;
	 * 
	 * spk::Widget* rootWidget = new spk::Widget();
	 * spk::Mouse mouse;
	 * spk::Keyboard keyboard;
	 * spk::Controller controller;
	 * 
	 * updateModule.linkToMouse(&mouse);
	 * updateModule.linkToKeyboard(&keyboard);
	 * updateModule.linkToController(&controller);
	 * updateModule.linkToWidget(rootWidget);
	 * 
	 * // Process update events
	 * updateModule.treatMessages();
	 * @endcode
	 */
	class UpdateModule : public spk::Module<spk::UpdateEvent>
	{
	private:
		spk::Widget* _rootWidget; /**< @brief Pointer to the root widget that receives update events. */
		const spk::Mouse* _mouse; /**< @brief Pointer to the mouse object for state updates. */
		const spk::Keyboard* _keyboard; /**< @brief Pointer to the keyboard object for state updates. */
		const spk::Controller* _controller; /**< @brief Pointer to the controller object for state updates. */

		/**
		 * @brief Processes a specific update event.
		 * 
		 * This method enriches the update event with the current input states and forwards it to the root widget.
		 * 
		 * @param p_event The update event to process.
		 */
		void _treatEvent(spk::UpdateEvent&& p_event) override;

		/**
		 * @brief Converts a generic event into an update event.
		 * 
		 * @param p_event The generic event to convert.
		 * @return The converted update event.
		 */
		spk::UpdateEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		/**
		 * @brief Constructs an `UpdateModule` instance.
		 */
		UpdateModule();

		/**
		 * @brief Links the module to a mouse object for update events.
		 * 
		 * @param p_mouse Pointer to the mouse object.
		 */
		void linkToMouse(const spk::Mouse* p_mouse);

		/**
		 * @brief Links the module to a keyboard object for update events.
		 * 
		 * @param p_keyboard Pointer to the keyboard object.
		 */
		void linkToKeyboard(const spk::Keyboard* p_keyboard);

		/**
		 * @brief Links the module to a controller object for update events.
		 * 
		 * @param p_controller Pointer to the controller object.
		 */
		void linkToController(const spk::Controller* p_controller);

		/**
		 * @brief Links the module to a root widget for update events.
		 * 
		 * This method sets the root widget to receive enriched update events.
		 * 
		 * @param p_rootWidget Pointer to the root widget.
		 */
		void linkToWidget(spk::Widget* p_rootWidget);
	};
}
