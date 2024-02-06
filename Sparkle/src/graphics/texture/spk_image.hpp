#pragma once

#include <filesystem>
#include "graphics/texture/spk_texture.hpp"

namespace spk
{
	/**
	 * @brief Extends the Texture class to load texture data from image files.
	 *
	 * The Image class is designed to facilitate the loading of textures from various image file formats
	 * into GPU memory. It automatically handles reading image files, extracting pixel data, determining
	 * the image's dimensions and color depth, and preparing the texture for use in rendering.
	 * 
	 * This class simplifies the process of using image files as textures in graphical applications.
	 *
	 * Upon instantiation, the Image class reads the specified image file, stores its data, and inherits
	 * the functionality to upload this data as a texture to the GPU.
	 * It supports all image formats that the underlying graphics API and libraries can parse.
	 *
	 * Usage example:
	 * @code
	 * spk::Image textureImage("path/to/texture.png");
	 * textureImage.uploadToGPU();
	 * 
	 * spk::Pipeline::texture("myTexture").attach(&textureImage);
	 * @endcode
	 *
	 * @note This class assumes that the filesystem path provided to the constructor points to a valid image file. The actual loading and decoding of the image file are performed upon construction, with the GPU upload occurring through the `uploadToGPU` method inherited from Texture.
	 *
	 * @see Texture, Pipeline::Texture
	 */
	class Image : public Texture
	{
	private:
		uint8_t *_textureData;
		spk::Vector2Int _size;
		int _channels;

	public:
		Image(const std::filesystem::path& p_path);
	};
}
