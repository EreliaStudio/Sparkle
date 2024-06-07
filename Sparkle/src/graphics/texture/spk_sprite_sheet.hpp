#pragma once

#include "graphics/texture/spk_image.hpp"

namespace spk
{
	/**
	 * @class SpriteSheet
	 * @brief Manages a texture divided into a grid of sprites, extending the Image class.
	 *
	 * The SpriteSheet class is designed for handling textures that contain multiple smaller images or sprites,
	 * arranged in a grid. It allows for easy access to individual sprites within the sheet based on their grid
	 * coordinates or an index.
	 * This is particularly useful in 2D game development and applications where multiple small graphics
	 * are packed into a single texture to improve performance and resource management.
	 *
	 * Upon construction, the SpriteSheet class calculates the dimensions of each sprite and the total number
	 * of sprites based on the provided sprite size and the dimensions of the underlying image.
	 * It provides methods to retrieve the position and size of individual sprites, enabling precise control over
	 * which part of the sprite sheet is used in rendering operations.
	 *
	 * Usage example:
	 * @code
	 * spk::SpriteSheet spriteSheet("path/to/spritesheet.png", spk::Vector2UInt(4, 4)); // Subdividing the texture in 4 sprites x 4 sprites
	 * spk::Vector2 spritePositionByPosition = spriteSheet.sprite(spk::Vector2UInt(2, 3)); // Get the position of the sprite at grid position (2,3), express between 0 and 1 for both axis
	 * spk::Vector2 spritePositionByIndex = spriteSheet.sprite(3); // Get the position of the 3nd sprite (Grid position (0,2)), express between 0 and 1 for both axis
	 * spk::Vector2 spriteSizeUnit = spriteSheet.unit(); // Get the size, express between 0 and 1, of a single sprite ((0.25, 0.25) in our case)
	 * @endcode
	 *
	 * @note The class assumes that all sprites within the sheet are of uniform size and are tightly packed without any padding between them. The sprite unit size and the total number of sprites are determined during object construction and cannot be changed afterward.
	 *
	 * @see Image, Vector2, Vector2UInt
	 */
	class SpriteSheet : public Image
	{
	private:
		spk::Vector2UInt _nbSprite;
		spk::Vector2 _unit;
		std::vector<spk::Vector2> _sprites;

	public:
		/**
		 * @brief Constructs a SpriteSheet object from an image file, subdivided into a grid of sprites.
		 * 
		 * This constructor loads the texture from the specified path and subdivides it into a grid based on the specified sprite size. It calculates the size of each sprite and the total number of sprites that fit within the image dimensions.
		 * 
		 * @param p_path The filesystem path to the spritesheet image file.
		 * @param p_spriteSize The size (width and height) of each sprite within the sheet, in pixels.
		 */
		SpriteSheet(const std::filesystem::path& p_path, const spk::Vector2UInt& p_spriteSize);

		/**
		 * @brief Gets the grid size of the spritesheet.
		 * 
		 * Returns the number of sprites horizontally and vertically within the spritesheet based on the sprite size and the overall image dimensions.
		 * 
		 * @return A Vector2UInt representing the grid size (columns, rows) of the spritesheet.
		 */
		const spk::Vector2UInt& nbSprite() const;

		/**
		 * @brief Gets the unit size of a single sprite within the spritesheet, expressed as a fraction of the whole image.
		 * 
		 * This method returns the size of a single sprite relative to the entire image dimensions, useful for texture mapping operations where coordinates are normalized.
		 * 
		 * @return A Vector2 representing the normalized width and height of a single sprite.
		 */
		const spk::Vector2& unit() const;

		/**
		 * @brief Gets a vector of all sprite positions within the spritesheet.
		 * 
		 * Each vector element represents the top-left corner of a sprite within the sheet, expressed in normalized texture coordinates (ranging from 0 to 1).
		 * 
		 * @return A vector of Vector2, each representing the normalized position of a sprite within the sheet.
		 */
		const std::vector<spk::Vector2>& sprites() const;

		/**
		 * @brief Calculates the sprite ID based on its grid coordinates.
		 * 
		 * This method translates grid coordinates (row and column) into a linear sprite ID, which can be used to retrieve the sprite's position or for indexing purposes.
		 * 
		 * @param p_spriteCoord A Vector2UInt representing the grid coordinates (column, row) of the sprite.
		 * @return The linear index (ID) of the sprite at the specified grid coordinates.
		 */
		size_t spriteID(const spk::Vector2UInt& p_spriteCoord) const;

		/**
		 * @brief Retrieves the normalized position of a sprite within the spritesheet by grid coordinates.
		 * 
		 * @param p_spriteCoord A Vector2UInt representing the grid coordinates (column, row) of the sprite.
		 * @return A Vector2 representing the normalized top-left corner position of the specified sprite.
		 */
		spk::Vector2 sprite(const spk::Vector2UInt& p_spriteCoord) const;

		/**
		 * @brief Retrieves the normalized position of a sprite within the spritesheet by sprite ID.
		 * 
		 * @param p_spriteID The linear index (ID) of the sprite.
		 * @return A Vector2 representing the normalized top-left corner position of the specified sprite.
		 */
		spk::Vector2 sprite(const size_t& p_spriteID) const;
	};
}
