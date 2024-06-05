#pragma once

#include <filesystem>
#include "graphics/texture/spk_texture.hpp"

namespace spk
{
	/**
	 * @class Image
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
		/**
		 * @brief Constructor for the Image class that loads image data from a file.
		 * 
		 * This constructor takes a filesystem path to an image file, reads the file, and initializes the Image object with its data. The image data includes pixel values, dimensions, and color depth information. This method is designed to handle various image file formats, automatically detecting and processing the image's properties.
		 * 
		 * The loaded image data is stored in memory and is ready to be uploaded to the GPU. The actual process of uploading to the GPU is done through the `uploadToGPU` method, which is inherited from the Texture class.
		 * 
		 * @param p_path The filesystem path to the image file. It should be a valid path to an image file that the system can read and decode.
		 *
		 * @note The path provided must point to a valid, readable image file. The supported formats depend on the underlying graphics API and libraries used for image parsing.
		 */
		Image(const std::filesystem::path& p_path);

		const spk::Vector2Int& size() const;
	};
}
