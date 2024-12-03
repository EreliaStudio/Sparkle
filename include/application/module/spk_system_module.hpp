#pragma once

#include "application/module/spk_module.hpp"
#include "structure/system/event/spk_event.hpp"
#include "spk_debug_macro.hpp"

namespace spk
{
	/**
	 * @class SystemModule
	 * @brief A module for handling system-level events.
	 * 
	 * The `SystemModule` listens for system events (e.g., window resizing, movement, or quitting),
	 * processes them, and interacts with the associated window to manage state changes.
	 * This class extends `spk::Module` and uses `spk::SystemEvent` as its event type.
	 * 
	 * Example usage:
	 * @code
	 * spk::SystemModule systemModule;
	 * 
	 * // Process system events
	 * systemModule.treatMessages();
	 * @endcode
	 */
	class SystemModule : public spk::Module<spk::SystemEvent>
	{
	private:
		/**
		 * @brief Processes a specific system event.
		 * 
		 * This method handles various system-level events such as resizing, moving, or quitting,
		 * and updates the associated window accordingly.
		 * 
		 * @param p_event The system event to process.
		 */
		void _treatEvent(spk::SystemEvent&& p_event) override;

		/**
		 * @brief Converts a generic event into a system event.
		 * 
		 * @param p_event The generic event to convert.
		 * @return The converted system event.
		 */
		spk::SystemEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		/**
		 * @brief Constructs a `SystemModule` instance.
		 */
		SystemModule();
	};
}
