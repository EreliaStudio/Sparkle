#pragma once

#include <filesystem>
#include "structure/math/spk_vector2.hpp"
#include "structure/graphics/opengl/spk_texture_object.hpp"

namespace spk
{
	/**
	 * @class Image
	 * @brief Represents an OpenGL texture object that loads and manages image data.
	 * 
	 * The `Image` class extends `OpenGL::TextureObject` to provide functionality for loading
	 * image files and managing image sections. It supports loading image data from files and
	 * setting texture properties such as filtering and wrapping.
	 * 
	 * Example usage:
	 * @code
	 * spk::Image image("path/to/image.png");
	 * image.loadFromFile("path/to/another/image.jpg");
	 * @endcode
	 */
	class Image : public OpenGL::TextureObject
	{
	public:
		/**
		 * @struct Section
		 * @brief Represents a section of an image defined by an anchor and size.
		 */
		struct Section
		{
			spk::Vector2 anchor; ///< The anchor point of the section.
			spk::Vector2 size;   ///< The size of the section.
		};

		/**
		 * @brief Constructs an `Image` and loads data from the specified file.
		 * @param filePath The path to the image file.
		 * @throws std::runtime_error If the file cannot be loaded.
		 */
		Image(const std::filesystem::path& filePath);

		/**
		 * @brief Loads image data from a file.
		 * @param filePath The path to the image file.
		 * @throws std::runtime_error If the file cannot be loaded.
		 */
		void loadFromFile(const std::filesystem::path& filePath);

	private:
		/**
		 * @brief Determines the texture format based on the number of image channels.
		 * @param channels The number of channels in the image.
		 * @return The corresponding `OpenGL::TextureObject::Format`.
		 */
		Format _determineFormat(int channels) const;
	};
}