#pragma once

#include "application/module/spk_module.hpp"
#include "structure/system/event/spk_event.hpp"
#include "spk_debug_macro.hpp"

namespace spk
{
	/**
	 * @class PaintModule
	 * @brief A module for handling paint events and managing the rendering pipeline.
	 * 
	 * The `PaintModule` listens for paint events triggered by windows, processes them, and forwards them
	 * to the linked widget for rendering. This class extends `spk::Module` and uses `spk::PaintEvent` as its event type.
	 * 
	 * Example usage:
	 * @code
	 * spk::PaintModule paintModule;
	 * spk::Widget* rootWidget = new spk::Widget();
	 * paintModule.linkToWidget(rootWidget);
	 * 
	 * // Process paint events
	 * paintModule.treatMessages();
	 * @endcode
	 */
	class PaintModule : public spk::Module<spk::PaintEvent>
	{
	private:
		spk::Widget* _rootWidget; /**< @brief Pointer to the root widget that receives paint events. */

		/**
		 * @brief Processes a specific paint event.
		 * 
		 * This method clears the window, triggers the widget's paint handling, and swaps the window buffers.
		 * 
		 * @param p_event The paint event to process.
		 */
		void _treatEvent(spk::PaintEvent&& p_event) override;

		/**
		 * @brief Converts a generic event into a paint event.
		 * 
		 * @param p_event The generic event to convert.
		 * @return The converted paint event.
		 */
		spk::PaintEvent _convertEventToEventType(spk::Event&& p_event) override;

	public:
		/**
		 * @brief Constructs a `PaintModule` instance.
		 */
		PaintModule();

		/**
		 * @brief Links the module to a root widget.
		 * 
		 * This method sets the root widget to receive processed paint events.
		 * 
		 * @param p_rootWidget Pointer to the root widget.
		 */
		void linkToWidget(spk::Widget* p_rootWidget);
	};
}
