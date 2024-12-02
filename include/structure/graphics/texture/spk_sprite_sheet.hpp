#pragma once

#include "structure/graphics/texture/spk_image.hpp"

namespace spk
{
	/**
	 * @class SpriteSheet
	 * @brief Represents a texture containing multiple sprites arranged in a grid.
	 * 
	 * The `SpriteSheet` class extends the `Image` class, providing additional functionality
	 * to manage and retrieve individual sprites based on their coordinates or ID.
	 * 
	 * Example usage:
	 * @code
	 * spk::Vector2UInt spriteSize(4, 4); // A 4x4 sprite sheet
	 * spk::SpriteSheet spriteSheet("path/to/spritesheet.png", spriteSize);
	 * 
	 * const auto& sprite = spriteSheet.sprite({1, 2}); // Get the sprite at row 2, column 1
	 * @endcode
	 */
	class SpriteSheet : public Image
	{
	public:
		/**
		 * @typedef Sprite
		 * @brief Defines a sprite as a section of the image.
		 */
		using Sprite = Image::Section;

	private:
		spk::Vector2UInt _nbSprite; ///< Number of sprites in the grid (columns x rows).
		spk::Vector2 _unit; ///< Size of each sprite as a fraction of the image.
		std::vector<Sprite> _sprites; ///< List of sprites with their anchor and size.

	public:
		/**
		 * @brief Constructs a `SpriteSheet` from an image file and sprite grid size.
		 * @param p_path Path to the image file.
		 * @param p_spriteSize Grid dimensions defining the number of sprites (columns x rows).
		 * @throws std::invalid_argument If the sprite size is (0, 0).
		 */
		SpriteSheet(const std::filesystem::path& p_path, const spk::Vector2UInt& p_spriteSize);

		/**
		 * @brief Gets the number of sprites in the grid.
		 * @return A `Vector2UInt` representing the number of columns and rows.
		 */
		const spk::Vector2UInt& nbSprite() const;

		/**
		 * @brief Gets the size of a single sprite as a fraction of the image.
		 * @return A `Vector2` representing the fractional size of each sprite.
		 */
		const spk::Vector2& unit() const;

		/**
		 * @brief Gets the list of all sprites.
		 * @return A reference to a vector containing all the sprites.
		 */
		const std::vector<Sprite>& sprites() const;

		/**
		 * @brief Gets the ID of a sprite based on its grid coordinates.
		 * @param p_spriteCoord The grid coordinates of the sprite (column, row).
		 * @return The ID of the sprite.
		 * @throws std::out_of_range If the coordinates are outside the sprite grid.
		 */
		size_t spriteID(const spk::Vector2UInt& p_spriteCoord) const;

		/**
		 * @brief Gets a sprite based on its grid coordinates.
		 * @param p_spriteCoord The grid coordinates of the sprite (column, row).
		 * @return A reference to the requested sprite.
		 * @throws std::out_of_range If the coordinates are outside the sprite grid.
		 */
		const Sprite& sprite(const spk::Vector2UInt& p_spriteCoord) const;

		/**
		 * @brief Gets a sprite based on its ID.
		 * @param p_spriteID The ID of the sprite.
		 * @return A reference to the requested sprite.
		 * @throws std::out_of_range If the sprite ID is invalid.
		 */
		const Sprite& sprite(const size_t& p_spriteID) const;
	};
}
