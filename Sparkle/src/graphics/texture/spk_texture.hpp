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
		Texture();

		Texture(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		virtual ~Texture();

		void uploadToGPU(const uint8_t* p_textureData, const spk::Vector2UInt& p_textureSize,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		void releaseGPUMemory();
	};
}
