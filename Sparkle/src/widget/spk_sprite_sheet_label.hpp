#pragma once

#include "widget/spk_widget.hpp"
#include "widget/components/spk_sprite_sheet_renderer.hpp"

namespace spk
{
	/**
	 * @class SpriteSheetLabel
	 * @brief A widget that displays an image from a sprite sheet.
	 *
	 * SpriteSheetLabel extends spk::Widget to provide functionality specifically for displaying images
	 * from a sprite sheet within a user interface. It utilizes the spk::WidgetComponent::SpriteSheetRenderer
	 * to handle the actual rendering of the sprite. This class is suitable for any UI elements that need to display
	 * images from a sprite sheet, such as icons, buttons, or animations.
	 *
	 * The class supports dynamic resizing and positioning, making it versatile for various UI layouts.
	 * It can be instantiated with a specific sprite during creation or have the sprite set or changed at runtime.
	 *
	 * Usage example:
	 * @code
	 * spk::SpriteSheetLabel spriteSheetLabel("spritesheet.png", parentWidget);
	 * spriteSheetLabel.label().setSpriteSheet(mySpriteSheet);
	 * spriteSheetLabel.label().setSprite(3); // Set the sprite by ID
	 * @endcode
	 */
	class SpriteSheetLabel : public spk::Widget
	{
	private:
		WidgetComponent::SpriteSheetRenderer _label; // Handles the rendering of the sprite.

		void _onGeometryChange();
		void _onRender();

	public:
		/**
		 * Constructs a SpriteSheetLabel widget with a parent.
		 * @param p_parent The parent widget of this SpriteSheetLabel.
		 */
		SpriteSheetLabel(spk::Widget* p_parent);

		/**
		 * Constructs a SpriteSheetLabel widget with a name and a parent.
		 * @param p_name The name of the widget, used for identification and debugging.
		 * @param p_parent The parent widget of this SpriteSheetLabel.
		 */
		SpriteSheetLabel(const std::string& p_name, spk::Widget* p_parent);

		/**
		 * Provides non-const access to the internal sprite sheet renderer component.
		 * @return Reference to the non-const SpriteSheetRenderer used by this SpriteSheetLabel.
		 */
		WidgetComponent::SpriteSheetRenderer& label();

		/**
		 * Provides const access to the internal sprite sheet renderer component.
		 * @return Reference to the const SpriteSheetRenderer used by this SpriteSheetLabel.
		 */
		const WidgetComponent::SpriteSheetRenderer& label() const;
	};
}