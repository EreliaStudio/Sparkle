#pragma once

#include "application/module/spk_module.hpp"
#include "structure/system/event/spk_event.hpp"
#include "spk_debug_macro.hpp"

namespace spk
{
	/**
	 * @class KeyboardModule
	 * @brief A module for managing keyboard events and tracking the state of the keyboard.
	 * 
	 * The `KeyboardModule` listens for keyboard events (key presses, releases, and glyph input),
	 * processes them, updates the keyboard state, and forwards them to a linked widget. This class
	 * extends `spk::Module` and uses `spk::KeyboardEvent` as its event type.
	 * 
	 * Example usage:
	 * @code
	 * spk::KeyboardModule keyboardModule;
	 * 
	 * spk::Widget* rootWidget = new spk::Widget();
	 * keyboardModule.linkToWidget(rootWidget);
	 * 
	 * // Process keyboard events
	 * keyboardModule.treatMessages();
	 * @endcode
	 */
	class KeyboardModule : public spk::Module<spk::KeyboardEvent>
	{
	private:
		spk::Widget* _rootWidget; /**< @brief Pointer to the root widget that receives keyboard events. */
		spk::Keyboard _keyboard; /**< @brief Stores the current state of the keyboard. */

		/**
		 * @brief Processes a specific keyboard event.
		 * 
		 * This method updates the keyboard state based on the event and forwards it to the root widget.
		 * 
		 * @param p_event The keyboard event to process.
		 */
		void _treatEvent(spk::KeyboardEvent&& p_event) override;

		/**
		 * @brief Converts a generic event into a keyboard event.
		 * 
		 * @param p_event The generic event to convert.
		 * @return The converted keyboard event.
		 */
		spk::KeyboardEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		/**
		 * @brief Constructs a `KeyboardModule` instance.
		 */
		KeyboardModule();

		/**
		 * @brief Links the module to a root widget.
		 * 
		 * This method sets the root widget to receive processed keyboard events.
		 * 
		 * @param p_rootWidget Pointer to the root widget.
		 */
		void linkToWidget(spk::Widget* p_rootWidget);

		/**
		 * @brief Retrieves the current state of the keyboard.
		 * 
		 * @return A reference to the `spk::Keyboard` object representing the current state.
		 */
		const spk::Keyboard& keyboard() const;
	};
}
