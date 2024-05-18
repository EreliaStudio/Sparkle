#pragma once

#include <unordered_map>
#include <string>
#include <filesystem>

#include "design_pattern/spk_singleton.hpp"
#include "graphics/texture/spk_image.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"

namespace spk
{
	/**
	 * @class TextureAtlas
	 * @brief Manages the loading, storage, and retrieval of textures, images, and sprite sheets.
	 *
	 * TextureAtlas provides a centralized system for managing various types of graphical resources
	 * like textures, images, and sprite sheets within the application. It ensures that each resource
	 * is loaded only once and provides an easy access point for retrieving these resources by name.
	 * This helps in optimizing memory usage and performance by avoiding redundant loads of the same
	 * texture and facilitating resource sharing across different parts of the application.
	 *
	 * Usage example:
	 * @code
	 * // Load a texture
	 * Texture* myTexture = textureManager.loadTexture("myTexture", data, size, Texture::Format::RGBA, Texture::Filtering::Linear,
	 *												 Texture::Wrap::Repeat, Texture::Mipmap::Default);
	 * // Retrieve a previously loaded texture
	 * Texture* retrievedTexture = textureManager.texture("myTexture");
	 * @endcode
	 *
	 * @note It is assumed that TextureAtlas follows the Singleton design pattern, ensuring a single instance.
	 */
	class TextureAtlas
	{
	private:
		std::unordered_map<std::string, Texture*> _loadedTextures; ///< Stores loaded textures keyed by their names.

	public:
		/**
		 * Constructor for TextureAtlas.
		 */
		TextureAtlas();

		/**
		 * Destructor for TextureAtlas.
		 */
		~TextureAtlas();
		
		/**
		 * Loads a texture from memory.
		 * @param p_textureName Unique name to associate with the texture.
		 * @param p_textureData Pointer to the texture data in memory.
		 * @param p_textureSize Size of the texture (width, height).
		 * @param p_format Format of the texture data (e.g., RGBA, RGB).
		 * @param p_filtering Texture filtering mode (e.g., Linear, Nearest).
		 * @param p_wrap Texture wrapping mode (e.g., Repeat, Clamp to Edge).
		 * @param p_mipmap Mipmap generation setting for the texture.
		 * @return Pointer to the newly loaded Texture object.
		 */
		Texture* loadTexture(const std::string& p_textureName,
							 const uint8_t* p_textureData, const Vector2UInt& p_textureSize,
							 const Texture::Format& p_format, const Texture::Filtering& p_filtering,
							 const Texture::Wrap& p_wrap, const Texture::Mipmap& p_mipmap);

		/**
		 * Loads an image from a file.
		 * @param p_textureName Unique name to associate with the image.
		 * @param p_path Filesystem path to the image file.
		 * @return Pointer to the newly loaded Image object.
		 */
		Image* loadImage(const std::string& p_textureName, const std::filesystem::path& p_path);

		/**
		 * Loads a sprite sheet from a file.
		 * @param p_textureName Unique name to associate with the sprite sheet.
		 * @param p_path Filesystem path to the sprite sheet file.
		 * @param p_spriteSheetSize Size of each sprite within the sheet (width, height).
		 * @return Pointer to the newly loaded SpriteSheet object.
		 */
		SpriteSheet* loadSpriteSheet(const std::string& p_textureName, 
									 const std::filesystem::path& p_path, const Vector2UInt& p_spriteSheetSize);

		/**
		 * Retrieves a loaded texture by name.
		 * @param p_textureName Name of the texture to retrieve.
		 * @return Pointer to the Texture object if found, nullptr otherwise.
		 */
		Texture* texture(const std::string& p_textureName) const;

		/**
		 * Retrieves a loaded image by name.
		 * @param p_textureName Name of the image to retrieve.
		 * @return Pointer to the Image object if found, nullptr otherwise.
		 */
		Image* image(const std::string& p_textureName) const;

		/**
		 * Retrieves a loaded sprite sheet by name.
		 * @param p_textureName Name of the sprite sheet to retrieve.
		 * @return Pointer to the SpriteSheet object if found, nullptr otherwise.
		 */
		SpriteSheet* spriteSheet(const std::string& p_textureName) const;
	};  
}
