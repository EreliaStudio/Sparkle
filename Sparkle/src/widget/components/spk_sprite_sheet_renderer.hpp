#pragma once

#include "widget/components/spk_texture_renderer.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"

namespace spk::WidgetComponent
{
	/**
	 * @class SpriteSheetRenderer
	 * @brief A widget component class for rendering sprites from a sprite sheet.
	 *
	 * This class extends TextureRenderer and is designed to handle the rendering of individual sprites from a 
	 * sprite sheet within the UI. It provides methods to set the sprite sheet, select specific sprites by their 
	 * coordinates or ID, and manage the necessary UV transformations for correct sprite rendering.
	 *
	 * SpriteSheetRenderer can be used to render animated or static sprites in the UI, allowing for efficient 
	 * management and rendering of multiple sprites from a single texture.
	 *
	 * Usage example:
	 * @code
	 * spk::SpriteSheet* mySpriteSheet = new spk::SpriteSheet(texture, spriteWidth, spriteHeight);
	 * spk::WidgetComponent::SpriteSheetRenderer* myRenderer = new spk::WidgetComponent::SpriteSheetRenderer();
	 * myRenderer->setSpriteSheet(mySpriteSheet);
	 * myRenderer->setSprite(spk::Vector2UInt(0, 0)); // Select the sprite at the top-left corner
	 * @endcode
	 *
	 * @note The class hides the setTexture and setTextureGeometry methods from the base TextureRenderer class
	 *       to enforce the use of sprite-specific methods for setting the texture and geometry.
	 */
	class SpriteSheetRenderer : public TextureRenderer
	{
	private:
		const spk::SpriteSheet* _spriteSheet; ///< The sprite sheet to render sprites from.
		size_t _spriteID; ///< The ID of the currently selected sprite.

		// Hide the setTexture and setTextureGeometry methods
		using TextureRenderer::setTexture;
		using TextureRenderer::setTextureGeometry;

	public:
		/**
		 * @brief Default constructor for SpriteSheetRenderer.
		 *
		 * Initializes a new instance of the SpriteSheetRenderer class, setting up the necessary rendering pipeline
		 * and preparing the object for sprite sheet rendering within the UI.
		 */
		SpriteSheetRenderer() :
			TextureRenderer(),
			_spriteSheet(nullptr),
			_spriteID(0)
		{}

		/**
		 * @brief Sets the sprite sheet to render sprites from.
		 * @param p_spriteSheet A pointer to the sprite sheet.
		 */
		void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
		{
			_spriteSheet = p_spriteSheet;
			updateSpriteUV();
		}

		/**
		 * @brief Sets the sprite to render by its coordinates within the sprite sheet.
		 * @param p_spriteCoord The coordinates of the sprite in the sprite sheet.
		 */
		void setSprite(const spk::Vector2UInt& p_spriteCoord)
		{
			_spriteID = _spriteSheet->spriteID(p_spriteCoord);
			updateSpriteUV();
		}

		/**
		 * @brief Sets the sprite to render by its ID within the sprite sheet.
		 * @param p_spriteID The ID of the sprite in the sprite sheet.
		 */
		void setSprite(const size_t& p_spriteID)
		{
			_spriteID = p_spriteID;
			updateSpriteUV();
		}

	private:
		/**
		 * @brief Updates the UV coordinates for the selected sprite.
		 *
		 * This method recalculates the UV coordinates based on the currently selected sprite ID and updates
		 * the texture geometry in the base TextureRenderer class.
		 */
		void updateSpriteUV()
		{
			if (_spriteSheet)
			{
				spk::Vector2 uvAnchor = _spriteSheet->sprite(_spriteID);
				spk::Vector2 uvSize = _spriteSheet->unit();
				TextureRenderer::setTextureGeometry(uvAnchor, uvSize);
			}
		}
	};
}
