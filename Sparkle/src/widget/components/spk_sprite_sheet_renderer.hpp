#pragma once

#include "widget/components/spk_texture_renderer.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"

namespace spk::WidgetComponent
{
	class SpriteSheetRenderer : public TextureRenderer
	{
	private:
		const spk::SpriteSheet* _spriteSheet;
		size_t _spriteID;

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

		void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
		{
			_spriteSheet = p_spriteSheet;
			updateSpriteUV();
		}

		void setSprite(const spk::Vector2UInt& p_spriteCoord)
		{
			_spriteID = _spriteSheet->spriteID(p_spriteCoord);
			updateSpriteUV();
		}

		void setSprite(const size_t& p_spriteID)
		{
			_spriteID = p_spriteID;
			updateSpriteUV();
		}

	private:
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
