#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/GL.h>

#include "math/spk_vector2.hpp"

namespace spk
{
	/**
	 * @class Texture
	 * @brief Manages texture data for use in rendering pipelines.
	 *
	 * This class encapsulates the functionality required to handle texture images, including loading them
	 * into GPU memory, setting their format, filtering modes, wrapping modes, and managing mipmaps.
	 * It supports various formats for texture data, such as RGB, RGBA, and greyscale, and provides options
	 * for how textures are sampled and how their edges are treated.
	 *
	 * Textures are essential for adding detail to rendered objects by applying image data to their
	 * surfaces. This class provides a straightforward interface for creating textures from image data,
	 * configuring their properties, and binding them for use in rendering operations.
	 *
	 * Usage example:
	 * @code
	 * spk::Texture texture(imageData, imageSize, spk::Texture::Format::RGBA, 
	 *                      spk::Texture::Filtering::Linear, spk::Texture::Wrap::Repeat, 
	 *                      spk::Texture::Mipmap::Enable);
	 * texture.uploadToGPU();
	 * @endcode
	 *
	 * @note Before a texture can be used in rendering, it must be uploaded to GPU memory using the `uploadToGPU` method.
	 * @note If data have been pushed to the GPU, it will be automaticaly release uppon destructor call or uppon re-upload to GPU.
	 *
	 * @see Pipeline, Vector2UInt
	 */
	class Texture
	{
	public:
		enum class Format
		{
			RGB = GL_RGB,
			RGBA = GL_RGBA,
			BGR = GL_BGR,
			BGRA = GL_BGRA,
			GreyLevel = GL_RED,
			Error = GL_NONE
		};

		enum class Filtering
		{
			Nearest = GL_NEAREST,
			Linear = GL_LINEAR
		};

		enum class Wrap
		{
			Repeat = GL_REPEAT,
			ClampToEdge = GL_CLAMP_TO_EDGE,
			ClampToBorder = GL_CLAMP_TO_BORDER
		};

		enum class Mipmap
		{
			Disable,
			Enable
		};
		
	private:
		bool _loaded;
		GLuint _textureID;

		friend class Pipeline;

		void _bind(int p_textureIndex = 0) const;
		void _unbind() const;

	public:
		/**
		 * @brief Default constructor for the Texture class.
		 * 
		 * Initializes a Texture object without any image data. Use `uploadToGPU` to load texture data into GPU memory.
		 */
		Texture();

		/**
		 * @brief Constructs a Texture with specified parameters and prepares it for GPU upload.
		 * 
		 * This constructor initializes a Texture object with the provided image data and configuration parameters, including format, filtering, wrap mode, and mipmap generation. However, it does not automatically upload the texture data to the GPU; use `uploadToGPU` for that purpose.
		 * 
		 * @param p_textureData Pointer to the raw texture data.
		 * @param p_textureSize Dimensions of the texture (width and height) as a Vector2UInt.
		 * @param p_format Format of the texture data (e.g., RGB, RGBA).
		 * @param p_filtering Filtering mode to use for the texture (e.g., Nearest, Linear).
		 * @param p_wrap Wrap mode for the texture (e.g., Repeat, ClampToEdge).
		 * @param p_mipmap Specifies whether to generate mipmaps (Enable or Disable).
		 */
		Texture(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		/**
		 * @brief Destructor for the Texture class.
		 * 
		 * Cleans up resources associated with the Texture object. If the texture has been uploaded to the GPU, it ensures that the corresponding GPU memory is released.
		 */
		virtual ~Texture();

		/**
		 * @brief Uploads texture data to GPU memory.
		 * 
		 * This method uploads the provided image data to GPU memory according to the specified format, filtering mode, wrap mode, and mipmap generation option. It is responsible for creating a GPU texture object and configuring it with the provided parameters.
		 * 
		 * @param p_textureData Pointer to the raw texture data.
		 * @param p_textureSize Dimensions of the texture (width and height) as a Vector2UInt.
		 * @param p_format Format of the texture data (e.g., RGB, RGBA).
		 * @param p_filtering Filtering mode to use for the texture (e.g., Nearest, Linear).
		 * @param p_wrap Wrap mode for the texture (e.g., Repeat, ClampToEdge).
		 * @param p_mipmap Specifies whether to generate mipmaps (Enable or Disable).
		 */
		void uploadToGPU(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		/**
		 * @brief Releases the texture's GPU memory.
		 * 
		 * If the texture has been uploaded to the GPU, this method releases the allocated GPU memory. This is useful for managing resources explicitly, especially in applications where textures are frequently created and destroyed.
		 */
		void releaseGPUMemory();
	};
}
