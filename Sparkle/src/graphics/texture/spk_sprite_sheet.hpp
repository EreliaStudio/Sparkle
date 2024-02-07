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
		spk::Vector2UInt _size;
		spk::Vector2 _unit;
		std::vector<spk::Vector2> _sprites;

	public:
		SpriteSheet(const std::filesystem::path& p_path, const spk::Vector2UInt& p_spriteSize);

		const spk::Vector2UInt& size() const;

		const spk::Vector2& unit() const;

		const std::vector<spk::Vector2>& sprites() const;

		size_t spriteID(const spk::Vector2UInt& p_spriteCoord) const;

		spk::Vector2 sprite(const spk::Vector2UInt& p_spriteCoord) const;
		spk::Vector2 sprite(const size_t& p_spriteID) const;
	};
}
